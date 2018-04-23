#include <scorum/account_statistics/account_statistics_plugin.hpp>
#include <scorum/account_statistics/account_statistics_api.hpp>
#include <scorum/common_statistics/base_plugin_impl.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/comment_objects.hpp>

#include <scorum/chain/database/database.hpp>

namespace scorum {
namespace account_statistics {

namespace detail {

class account_statistics_plugin_impl
    : public common_statistics::common_statistics_plugin_impl<bucket_object, account_statistics_plugin>
{
public:
    account_statistics_plugin_impl(account_statistics_plugin& plugin)
        : base_plugin_impl(plugin)
    {
    }
    virtual ~account_statistics_plugin_impl()
    {
    }

    virtual void process_post_operation(const bucket_object& bucket, const operation_notification& o) override;
};

struct activity_operation_process
{
    chain::database& _db;
    const activity_bucket_object& _bucket;

    activity_operation_process(chain::database& db, const activity_bucket_object& b)
        : _db(db)
        , _bucket(b)
    {
    }

    typedef void result_type;

    template <typename T> void operator()(const T&) const
    {
    }
};

struct operation_process
{
    chain::database& _db;
    const bucket_object& _bucket;

    operation_process(chain::database& db, const bucket_object& b)
        : _db(db)
        , _bucket(b)
    {
    }

    typedef void result_type;

    template <typename T> void operator()(const T&) const
    {
    }

    void operator()(const transfer_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) {
            auto& from_stat = b.account_statistic[op.from];
            from_stat.transfers_from++;
            from_stat.scorum_sent += op.amount;

            auto& to_stat = b.account_statistic[op.to];
            to_stat.transfers_to++;
            to_stat.scorum_received += op.amount;
        });
    }
};

void account_statistics_plugin_impl::process_post_operation(const bucket_object& bucket,
                                                            const operation_notification& o)
{
    auto& db = _self.database();

    o.op.visit(operation_process(db, bucket));
}

} // namespace detail

account_statistics_plugin::account_statistics_plugin(application* app)
    : plugin(app)
    , _my(new detail::account_statistics_plugin_impl(*this))
{
}

account_statistics_plugin::~account_statistics_plugin()
{
}

void account_statistics_plugin::plugin_set_program_options(boost::program_options::options_description& cli,
                                                           boost::program_options::options_description& cfg)
{
    cli.add_options()(
        "account-stats-bucket-size",
        boost::program_options::value<std::string>()->default_value("[60,3600,21600,86400,604800,2592000]"),
        "Track account statistics by grouping orders into buckets of equal size measured in seconds specified as a "
        "JSON array of numbers")(
        "account-stats-history-per-bucket", boost::program_options::value<uint32_t>()->default_value(100),
        "How far back in time to track history for each bucker size, measured in the number of buckets (default: 100)")(
        "account-stats-tracked-accounts", boost::program_options::value<std::string>()->default_value("[]"),
        "Which accounts to track the statistics of. Empty list tracks all accounts.");
    cfg.add(cli);
}

void account_statistics_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    try
    {
        _my->initialize();
    }
    FC_LOG_AND_RETHROW()

    print_greeting();
}

void account_statistics_plugin::plugin_startup()
{
    app().register_api_factory<account_statistics_api>("account_stats_api");
}

const flat_set<uint32_t>& account_statistics_plugin::get_tracked_buckets() const
{
    return _my->_tracked_buckets;
}

uint32_t account_statistics_plugin::get_max_history_per_bucket() const
{
    return _my->_maximum_history_per_bucket_size;
}
} // namespace account_statistics
} // namespace scorum

SCORUM_DEFINE_PLUGIN(account_statistics, scorum::account_statistics::account_statistics_plugin);
