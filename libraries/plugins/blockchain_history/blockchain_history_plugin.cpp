#include <scorum/blockchain_history/blockchain_history_plugin.hpp>
#include <scorum/blockchain_history/account_history_api.hpp>
#include <scorum/blockchain_history/blockchain_history_api.hpp>
#include <scorum/blockchain_history/schema/account_history_object.hpp>

#include <scorum/app/impacted.hpp>

#include <scorum/protocol/config.hpp>

#include <scorum/chain/database/database.hpp>
#include <scorum/chain/operation_notification.hpp>
#include <scorum/blockchain_history/schema/operation_objects.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <boost/algorithm/string.hpp>

#define SCORUM_NAMESPACE_PREFIX "scorum::protocol::"

namespace scorum {
namespace blockchain_history {

namespace detail {

using namespace scorum::protocol;

class blockchain_history_plugin_impl
{
public:
    blockchain_history_plugin_impl(blockchain_history_plugin& _plugin)
        : _self(_plugin)
    {
        chain::database& db = database();

        db.add_plugin_index<operation_index>();
        db.add_plugin_index<account_operations_full_history_index>();
        db.add_plugin_index<transfers_to_scr_history_index>();
        db.add_plugin_index<transfers_to_sp_history_index>();
        db.add_plugin_index<filtered_not_virt_operations_history_index>();
        db.add_plugin_index<filtered_virt_operations_history_index>();
        db.add_plugin_index<filtered_market_operations_history_index>();

        db.pre_apply_operation.connect([&](const operation_notification& note) { on_operation(note); });
    }
    virtual ~blockchain_history_plugin_impl()
    {
    }

    scorum::chain::database& database()
    {
        return _self.database();
    }

    const operation_object& create_operation_obj(const operation_notification& note);
    void update_filtered_operation_index(const operation_object& object, const operation& op);
    void on_operation(const operation_notification& note);

    blockchain_history_plugin& _self;
    flat_map<account_name_type, account_name_type> _tracked_accounts;
    bool _filter_content = false;
    bool _blacklist = false;
    flat_set<std::string> _op_list;
};

class operation_visitor
{
    database& _db;
    const operation_object& _obj;
    account_name_type _item;

public:
    using result_type = void;

    operation_visitor(database& db, const operation_object& obj, const account_name_type& i)
        : _db(db)
        , _obj(obj)
        , _item(i)
    {
    }

    template <typename Op> void operator()(const Op&) const
    {
        push_history<account_history_object>(_obj);
    }

    void operator()(const transfer_operation&) const
    {
        push_history<account_history_object>(_obj);
        push_history<transfers_to_scr_history_object>(_obj);
    }

    void operator()(const transfer_to_scorumpower_operation&) const
    {
        push_history<account_history_object>(_obj);
        push_history<transfers_to_sp_history_object>(_obj);
    }

private:
    template <typename history_object_type> void push_history(const operation_object& op) const
    {
        const auto& hist_idx = _db.get_index<history_index<history_object_type>>().indices().get<by_account>();
        auto hist_itr = hist_idx.lower_bound(boost::make_tuple(_item, uint32_t(-1)));
        uint32_t sequence = 0;
        if (hist_itr != hist_idx.end() && hist_itr->account == _item)
            sequence = hist_itr->sequence + 1;

        _db.create<history_object_type>([&](history_object_type& ahist) {
            ahist.account = _item;
            ahist.sequence = sequence;
            ahist.op = op.id;
        });
    }
};

struct operation_visitor_filter
{
    operation_visitor_filter(const flat_set<std::string>& filter, bool blacklist)
        : _filter(filter)
        , _blacklist(blacklist)
    {
    }

    using result_type = bool;

    template <typename T> bool operator()(const T& op) const
    {
        if (_filter.find(fc::get_typename<T>::name()) != _filter.end())
        {
            if (!_blacklist)
                return true;
        }
        else
        {
            if (_blacklist)
                return true;
        }

        return false;
    }

private:
    const flat_set<std::string>& _filter;
    bool _blacklist;
};

const operation_object& blockchain_history_plugin_impl::create_operation_obj(const operation_notification& note)
{
    scorum::chain::database& db = database();

    return db.create<operation_object>([&](operation_object& obj) {
        obj.trx_id = note.trx_id;
        obj.block = note.block;
        obj.trx_in_block = note.trx_in_block;
        obj.op_in_trx = note.op_in_trx;
        obj.timestamp = db.head_block_time();
        auto size = fc::raw::pack_size(note.op);
        obj.serialized_op.resize(size);
        fc::datastream<char*> ds(obj.serialized_op.data(), size);
        fc::raw::pack(ds, note.op);
    });
}

void blockchain_history_plugin_impl::update_filtered_operation_index(const operation_object& object,
                                                                     const operation& op)
{
    scorum::chain::database& db = database();

    if (is_virtual_operation(op))
    {
        db.create<filtered_virt_operations_history_object>(
            [&](filtered_virt_operations_history_object& obj) { obj.op = object.id; });
    }
    else
    {
        db.create<filtered_not_virt_operations_history_object>(
            [&](filtered_not_virt_operations_history_object& obj) { obj.op = object.id; });
    }
    if (is_market_operation(op))
    {
        db.create<filtered_market_operations_history_object>(
            [&](filtered_market_operations_history_object& obj) { obj.op = object.id; });
    }
}

void blockchain_history_plugin_impl::on_operation(const operation_notification& note)
{
    flat_set<account_name_type> impacted;
    scorum::chain::database& db = database();

    if (_filter_content && !note.op.visit(operation_visitor_filter(_op_list, _blacklist)))
        return;

    app::operation_get_impacted_accounts(note.op, impacted);

    const operation_object& new_obj = create_operation_obj(note);
    update_filtered_operation_index(new_obj, note.op);
    for (const auto& item : impacted)
    {
        auto itr = _tracked_accounts.lower_bound(item);

        /*
         * The map containing the ranges uses the key as the lower bound and the value as the upper bound.
         * Because of this, if a value exists with the range (key, value], then calling lower_bound on
         * the map will return the key of the next pair. Under normal circumstances of those ranges not
         * intersecting, the value we are looking for will not be present in range that is returned via
         * lower_bound.
         *
         * Consider the following example using ranges ["a","c"], ["g","i"]
         * If we are looking for "bob", it should be tracked because it is in the lower bound.
         * However, lower_bound( "bob" ) returns an iterator to ["g","i"]. So we need to decrement the iterator
         * to get the correct range.
         *
         * If we are looking for "g", lower_bound( "g" ) will return ["g","i"], so we need to make sure we don't
         * decrement.
         *
         * If the iterator points to the end, we should check the previous (equivalent to rbegin)
         *
         * And finally if the iterator is at the beginning, we should not decrement it for obvious reasons
         */
        if (itr != _tracked_accounts.begin()
            && ((itr != _tracked_accounts.end() && itr->first != item) || itr == _tracked_accounts.end()))
        {
            --itr;
        }

        if (!_tracked_accounts.size() || (itr != _tracked_accounts.end() && itr->first <= item && item <= itr->second))
        {
            note.op.visit(operation_visitor(db, new_obj, item));
        }
    }
}

} // end namespace detail

blockchain_history_plugin::blockchain_history_plugin(application* app)
    : plugin(app)
    , my(new detail::blockchain_history_plugin_impl(*this))
{
    // ilog("Loading account history plugin" );
}

blockchain_history_plugin::~blockchain_history_plugin()
{
}

std::string blockchain_history_plugin::plugin_name() const
{
    return BLOCKCHAIN_HISTORY_PLUGIN_NAME;
}

void blockchain_history_plugin::plugin_set_program_options(boost::program_options::options_description& cli,
                                                           boost::program_options::options_description& cfg)
{
    cli.add_options()(
        "track-account-range", boost::program_options::value<std::vector<std::string>>()->composing()->multitoken(),
        "Defines a range of accounts to track as a json pair [\"from\",\"to\"] [from,to] Can be specified multiple "
        "times")("history-whitelist-ops", boost::program_options::value<std::vector<std::string>>()->composing(),
                 "Defines a list of operations which will be explicitly logged.")(
        "history-blacklist-ops", boost::program_options::value<std::vector<std::string>>()->composing(),
        "Defines a list of operations which will be explicitly ignored.");
    cfg.add(cli);
}

void blockchain_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    typedef std::pair<account_name_type, account_name_type> pairstring;
    LOAD_VALUE_SET(options, "track-account-range", my->_tracked_accounts, pairstring);

    if (options.count("history-whitelist-ops"))
    {
        my->_filter_content = true;
        my->_blacklist = false;

        for (auto& arg : options.at("history-whitelist-ops").as<std::vector<std::string>>())
        {
            std::vector<std::string> ops;
            boost::split(ops, arg, boost::is_any_of(" \t,"));

            for (const std::string& op : ops)
            {
                if (op.size())
                    my->_op_list.insert(SCORUM_NAMESPACE_PREFIX + op);
            }
        }

        ilog("Account History: whitelisting ops ${o}", ("o", my->_op_list));
    }
    else if (options.count("history-blacklist-ops"))
    {
        my->_filter_content = true;
        my->_blacklist = true;
        for (auto& arg : options.at("history-blacklist-ops").as<std::vector<std::string>>())
        {
            std::vector<std::string> ops;
            boost::split(ops, arg, boost::is_any_of(" \t,"));

            for (const std::string& op : ops)
            {
                if (op.size())
                    my->_op_list.insert(SCORUM_NAMESPACE_PREFIX + op);
            }
        }

        ilog("Account History: blacklisting ops ${o}", ("o", my->_op_list));
    }
    print_greeting();
}

void blockchain_history_plugin::plugin_startup()
{
    ilog("account_history plugin: plugin_startup() begin");

    app().register_api_factory<account_history_api>("account_history_api");
    app().register_api_factory<blockchain_history_api>("blockchain_history_api");

    ilog("account_history plugin: plugin_startup() end");
}

flat_map<account_name_type, account_name_type> blockchain_history_plugin::tracked_accounts() const
{
    return my->_tracked_accounts;
}
}
}

SCORUM_DEFINE_PLUGIN(blockchain_history, scorum::blockchain_history::blockchain_history_plugin)
