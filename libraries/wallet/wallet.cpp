#include <graphene/utilities/git_revision.hpp>
#include <graphene/utilities/key_conversion.hpp>

#include <scorum/app/api.hpp>
#include <scorum/app/chain_api.hpp>
#include <scorum/protocol/base.hpp>
#include <scorum/wallet/wallet.hpp>
#include <scorum/wallet/api_documentation.hpp>
#include <scorum/wallet/reflect_util.hpp>

#include <scorum/account_by_key/account_by_key_api.hpp>
#include <scorum/blockchain_history/account_history_api.hpp>
#include <scorum/blockchain_history/blockchain_history_api.hpp>

#include <scorum/protocol/atomicswap_helper.hpp>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <list>
#include <cstdlib>

#include <boost/version.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm/sort.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <fc/container/deque.hpp>
#include <fc/git_revision.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/stdio.hpp>
#include <fc/network/http/websocket.hpp>
#include <fc/rpc/websocket_api.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/thread/mutex.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <fc/smart_ref_impl.hpp>

#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <scorum/cli/formatter.hpp>

namespace scorum {
namespace wallet {

using namespace graphene::utilities;

namespace detail {

template <class T> optional<T> maybe_id(const std::string& name_or_id)
{
    if (std::isdigit(name_or_id.front()))
    {
        try
        {
            return fc::variant(name_or_id).as<T>();
        }
        catch (const fc::exception&)
        {
        }
    }
    return optional<T>();
}

struct op_prototype_visitor
{
    typedef void result_type;

    int t = 0;
    flat_map<std::string, operation>& name2op;

    op_prototype_visitor(int _t, flat_map<std::string, operation>& _prototype_ops)
        : t(_t)
        , name2op(_prototype_ops)
    {
    }

    template <typename Type> result_type operator()(const Type& op) const
    {
        std::string name = fc::get_typename<Type>::name();
        size_t p = name.rfind(':');
        if (p != std::string::npos)
            name = name.substr(p + 1);
        name2op[name] = Type();
    }
};

class wallet_api_impl
{
public:
    api_documentation method_documentation;

private:
    void enable_umask_protection()
    {
#ifdef __unix__
        _old_umask = umask(S_IRWXG | S_IRWXO);
#endif
    }

    void disable_umask_protection()
    {
#ifdef __unix__
        umask(_old_umask);
#endif
    }

    void init_prototype_ops()
    {
        operation op;
        for (int t = 0; t < op.count(); t++)
        {
            op.set_which(t);
            op.visit(op_prototype_visitor(t, _prototype_ops));
        }
    }

public:
    wallet_api& self;
    wallet_api_impl(wallet_api& s, const wallet_data& initial_data, fc::api<login_api> rapi)
        : self(s)
        , _chain_id(initial_data.chain_id)
        , _remote_api(rapi)
        , _remote_db(rapi->get_api_by_name("database_api")->as<database_api>())
        , _chain_api(rapi->get_api_by_name(API_CHAIN)->as<chain_api>())
        , _remote_net_broadcast(rapi->get_api_by_name("network_broadcast_api")->as<network_broadcast_api>())
    {
        init_prototype_ops();

        chain_id_type remote_chain_id = _remote_db->get_chain_id();
        if (remote_chain_id != _chain_id)
        {
            FC_THROW("Remote server gave us an unexpected chain_id",
                     ("remote_chain_id", remote_chain_id)("chain_id", _chain_id));
        }

        _wallet.ws_server = initial_data.ws_server;
        _wallet.ws_user = initial_data.ws_user;
        _wallet.ws_password = initial_data.ws_password;
        _wallet.chain_id = initial_data.chain_id;
    }

    virtual ~wallet_api_impl()
    {
    }

    void encrypt_keys()
    {
        if (!is_locked())
        {
            plain_keys data;
            data.keys = _keys;
            data.checksum = _checksum;
            auto plain_txt = fc::raw::pack(data);
            _wallet.cipher_keys = fc::aes_encrypt(data.checksum, plain_txt);
        }
    }

    bool copy_wallet_file(const std::string& destination_filename)
    {
        fc::path src_path = get_wallet_filename();
        if (!fc::exists(src_path))
            return false;
        fc::path dest_path = destination_filename + _wallet_filename_extension;
        int suffix = 0;
        while (fc::exists(dest_path))
        {
            ++suffix;
            dest_path = destination_filename + "-" + std::to_string(suffix) + _wallet_filename_extension;
        }
        wlog("backing up wallet ${src} to ${dest}", ("src", src_path)("dest", dest_path));

        fc::path dest_parent = fc::absolute(dest_path).parent_path();
        try
        {
            enable_umask_protection();
            if (!fc::exists(dest_parent))
                fc::create_directories(dest_parent);
            fc::copy(src_path, dest_path);
            disable_umask_protection();
        }
        catch (...)
        {
            disable_umask_protection();
            throw;
        }
        return true;
    }

    bool is_locked() const
    {
        return _checksum == fc::sha512();
    }

    variant info() const
    {
        auto dynamic_props = _remote_db->get_dynamic_global_properties();

        fc::mutable_variant_object result = fc::variant(dynamic_props).get_object();

        result["witness_majority_version"] = fc::string(dynamic_props.majority_version);
        result["hardfork_version"] = fc::string(_chain_api->get_chain_properties().hf_version);

        result["chain_properties"] = fc::variant(dynamic_props.median_chain_props).get_object();

        result["chain_id"] = _chain_id;
        result["head_block_age"]
            = fc::get_approximate_relative_time_string(dynamic_props.time, time_point_sec(time_point::now()), " old");

        result["participation"] = (100 * dynamic_props.recent_slots_filled.popcount()) / 128.0;

        return result;
    }

    variant_object about() const
    {
        std::string client_version(graphene::utilities::git_revision_description);
        const size_t pos = client_version.find('/');
        if (pos != std::string::npos && client_version.size() > pos)
            client_version = client_version.substr(pos + 1);

        fc::mutable_variant_object result;
        result["blockchain_version"] = SCORUM_BLOCKCHAIN_VERSION;
        result["client_version"] = client_version;
        result["scorum_revision"] = graphene::utilities::git_revision_sha;
        result["scorum_revision_age"] = fc::get_approximate_relative_time_string(
            fc::time_point_sec(graphene::utilities::git_revision_unix_timestamp));
        result["fc_revision"] = fc::git_revision_sha;
        result["fc_revision_age"]
            = fc::get_approximate_relative_time_string(fc::time_point_sec(fc::git_revision_unix_timestamp));
        result["compile_date"] = "compiled on " __DATE__ " at " __TIME__;
        result["boost_version"] = boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".");
        result["openssl_version"] = OPENSSL_VERSION_TEXT;

        std::string bitness = boost::lexical_cast<std::string>(8 * sizeof(int*)) + "-bit";
#if defined(__APPLE__)
        std::string os = "osx";
#elif defined(__linux__)
        std::string os = "linux";
#elif defined(_MSC_VER)
        std::string os = "win32";
#else
        std::string os = "other";
#endif
        result["build"] = os + " " + bitness;

        try
        {
            auto v = _remote_api->get_version();
            result["server_blockchain_version"] = v.blockchain_version;
            result["server_scorum_revision"] = v.scorum_revision;
            result["server_fc_revision"] = v.fc_revision;
        }
        catch (fc::exception&)
        {
            result["server"] = "could not retrieve server version information";
        }

        return result;
    }

    account_api_obj get_account(const std::string& account_name) const
    {
        auto accounts = _remote_db->get_accounts({ account_name });
        FC_ASSERT(!accounts.empty(), "Unknown account");
        return accounts.front();
    }

    std::string get_wallet_filename() const
    {
        return _wallet_filename;
    }

    optional<fc::ecc::private_key> try_get_private_key(const public_key_type& id) const
    {
        auto it = _keys.find(id);
        if (it != _keys.end())
            return wif_to_key(it->second);
        return optional<fc::ecc::private_key>();
    }

    fc::ecc::private_key get_private_key(const public_key_type& id) const
    {
        auto has_key = try_get_private_key(id);
        FC_ASSERT(has_key);
        return *has_key;
    }

    fc::ecc::private_key get_private_key_for_account(const account_api_obj& account) const
    {
        std::vector<public_key_type> active_keys = account.active.get_keys();
        if (active_keys.size() != 1)
            FC_THROW("Expecting a simple authority with one active key");
        return get_private_key(active_keys.front());
    }

    // imports the private key into the wallet, and associate it in some way (?) with the
    // given account name.
    // @returns true if the key matches a current active/owner/memo key for the named
    //          account, false otherwise (but it is stored either way)
    bool import_key(const std::string& wif_key)
    {
        fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
        if (!optional_private_key)
            FC_THROW("Invalid private key");
        scorum::chain::public_key_type wif_pub_key = optional_private_key->get_public_key();

        _keys[wif_pub_key] = wif_key;
        return true;
    }

    // TODO: Needs refactoring
    bool load_wallet_file(std::string wallet_filename = "")
    {
        // TODO:  Merge imported wallet with existing wallet,
        //        instead of replacing it
        if (wallet_filename == "")
            wallet_filename = _wallet_filename;

        if (!fc::exists(wallet_filename))
            return false;

        _wallet = fc::json::from_file(wallet_filename).as<wallet_data>();

        return true;
    }

    // TODO: Needs refactoring
    void save_wallet_file(std::string wallet_filename = "")
    {
        //
        // Serialize in memory, then save to disk
        //
        // This approach lessens the risk of a partially written wallet
        // if exceptions are thrown in serialization
        //

        encrypt_keys();

        if (wallet_filename == "")
            wallet_filename = _wallet_filename;

        wlog("saving wallet to file ${fn}", ("fn", wallet_filename));

        std::string data = fc::json::to_pretty_string(_wallet);
        try
        {
            enable_umask_protection();
            //
            // Parentheses on the following declaration fails to compile,
            // due to the Most Vexing Parse.  Thanks, C++
            //
            // http://en.wikipedia.org/wiki/Most_vexing_parse
            //
            fc::ofstream outfile{ fc::path(wallet_filename) };
            outfile.write(data.c_str(), data.length());
            outfile.flush();
            outfile.close();
            disable_umask_protection();
        }
        catch (...)
        {
            disable_umask_protection();
            throw;
        }
    }

    // This function generates derived keys starting with index 0 and keeps incrementing
    // the index until it finds a key that isn't registered in the block chain.  To be
    // safer, it continues checking for a few more keys to make sure there wasn't a short gap
    // caused by a failed registration or the like.
    int find_first_unused_derived_key_index(const fc::ecc::private_key& parent_key)
    {
        int first_unused_index = 0;
        int number_of_consecutive_unused_keys = 0;
        for (int key_index = 0;; ++key_index)
        {
            fc::ecc::private_key derived_private_key = derive_private_key(key_to_wif(parent_key), key_index);
            scorum::chain::public_key_type derived_public_key = derived_private_key.get_public_key();
            if (_keys.find(derived_public_key) == _keys.end())
            {
                if (number_of_consecutive_unused_keys)
                {
                    ++number_of_consecutive_unused_keys;
                    if (number_of_consecutive_unused_keys > 5)
                        return first_unused_index;
                }
                else
                {
                    first_unused_index = key_index;
                    number_of_consecutive_unused_keys = 1;
                }
            }
            else
            {
                // key_index is used
                first_unused_index = 0;
                number_of_consecutive_unused_keys = 0;
            }
        }
    }

    signed_transaction create_account_with_private_key(fc::ecc::private_key owner_privkey,
                                                       const std::string& account_name,
                                                       const std::string& creator_account_name,
                                                       bool broadcast = false,
                                                       bool save_wallet = true)
    {
        try
        {
            int active_key_index = find_first_unused_derived_key_index(owner_privkey);
            fc::ecc::private_key active_privkey = derive_private_key(key_to_wif(owner_privkey), active_key_index);

            int memo_key_index = find_first_unused_derived_key_index(active_privkey);
            fc::ecc::private_key memo_privkey = derive_private_key(key_to_wif(active_privkey), memo_key_index);

            scorum::chain::public_key_type owner_pubkey = owner_privkey.get_public_key();
            scorum::chain::public_key_type active_pubkey = active_privkey.get_public_key();
            scorum::chain::public_key_type memo_pubkey = memo_privkey.get_public_key();

            account_create_operation account_create_op;

            account_create_op.creator = creator_account_name;
            account_create_op.new_account_name = account_name;
            account_create_op.fee = _chain_api->get_chain_properties().median_chain_props.account_creation_fee;
            account_create_op.owner = authority(1, owner_pubkey, 1);
            account_create_op.active = authority(1, active_pubkey, 1);
            account_create_op.memo_key = memo_pubkey;

            signed_transaction tx;

            tx.operations.push_back(account_create_op);
            tx.validate();

            if (save_wallet)
                save_wallet_file();
            if (broadcast)
            {
                //_remote_net_broadcast->broadcast_transaction( tx );
                auto result = _remote_net_broadcast->broadcast_transaction_synchronous(tx);
            }
            return tx;
        }
        FC_CAPTURE_AND_RETHROW((account_name)(creator_account_name)(broadcast)(save_wallet))
    }

    signed_transaction
    set_voting_proxy(const std::string& account_to_modify, const std::string& proxy, bool broadcast /* = false */)
    {
        try
        {
            account_witness_proxy_operation op;
            op.account = account_to_modify;
            op.proxy = proxy;

            signed_transaction tx;
            tx.operations.push_back(op);
            tx.validate();

            return sign_transaction(tx, broadcast);
        }
        FC_CAPTURE_AND_RETHROW((account_to_modify)(proxy)(broadcast))
    }

    optional<witness_api_obj> get_witness(const std::string& owner_account)
    {
        return _remote_db->get_witness_by_account(owner_account);
    }

    void set_transaction_expiration(uint32_t tx_expiration_seconds)
    {
        FC_ASSERT(tx_expiration_seconds < SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        _tx_expiration_seconds = tx_expiration_seconds;
    }

    annotated_signed_transaction sign_transaction(signed_transaction tx, bool broadcast = false)
    {
        flat_set<account_name_type> req_active_approvals;
        flat_set<account_name_type> req_owner_approvals;
        flat_set<account_name_type> req_posting_approvals;
        std::vector<authority> other_auths;

        tx.get_required_authorities(req_active_approvals, req_owner_approvals, req_posting_approvals, other_auths);

        for (const auto& auth : other_auths)
            for (const auto& a : auth.account_auths)
                req_active_approvals.insert(a.first);

        // std::merge lets us de-duplicate account_id's that occur in both
        //   sets, and dump them into a vector (as required by remote_db api)
        //   at the same time
        std::vector<std::string> v_approving_account_names;
        std::merge(req_active_approvals.begin(), req_active_approvals.end(), req_owner_approvals.begin(),
                   req_owner_approvals.end(), std::back_inserter(v_approving_account_names));

        for (const auto& a : req_posting_approvals)
            v_approving_account_names.push_back(a);

        /// TODO: fetch the accounts specified via other_auths as well.

        auto approving_account_objects = _remote_db->get_accounts(v_approving_account_names);

        /// TODO: recursively check one layer deeper in the authority tree for keys

        FC_ASSERT(approving_account_objects.size() == v_approving_account_names.size(), "",
                  ("aco.size:", approving_account_objects.size())("acn", v_approving_account_names.size()));

        flat_map<std::string, account_api_obj> approving_account_lut;
        size_t i = 0;
        for (const optional<account_api_obj>& approving_acct : approving_account_objects)
        {
            if (!approving_acct.valid())
            {
                wlog("operation_get_required_auths said approval of non-existing account ${name} was needed",
                     ("name", v_approving_account_names[i]));
                i++;
                continue;
            }
            approving_account_lut[approving_acct->name] = *approving_acct;
            i++;
        }

        flat_set<public_key_type> approving_key_set;
        for (account_name_type& acct_name : req_active_approvals)
        {
            const auto it = approving_account_lut.find(acct_name);
            if (it == approving_account_lut.end())
                continue;
            const account_api_obj& acct = it->second;
            std::vector<public_key_type> v_approving_keys = acct.active.get_keys();
            wdump((v_approving_keys));
            for (const public_key_type& approving_key : v_approving_keys)
            {
                wdump((approving_key));
                approving_key_set.insert(approving_key);
            }
        }

        for (account_name_type& acct_name : req_posting_approvals)
        {
            const auto it = approving_account_lut.find(acct_name);
            if (it == approving_account_lut.end())
                continue;
            const account_api_obj& acct = it->second;
            std::vector<public_key_type> v_approving_keys = acct.posting.get_keys();
            wdump((v_approving_keys));
            for (const public_key_type& approving_key : v_approving_keys)
            {
                wdump((approving_key));
                approving_key_set.insert(approving_key);
            }
        }

        for (const account_name_type& acct_name : req_owner_approvals)
        {
            const auto it = approving_account_lut.find(acct_name);
            if (it == approving_account_lut.end())
                continue;
            const account_api_obj& acct = it->second;
            std::vector<public_key_type> v_approving_keys = acct.owner.get_keys();
            for (const public_key_type& approving_key : v_approving_keys)
            {
                wdump((approving_key));
                approving_key_set.insert(approving_key);
            }
        }
        for (const authority& a : other_auths)
        {
            for (const auto& k : a.key_auths)
            {
                wdump((k.first));
                approving_key_set.insert(k.first);
            }
        }

        auto dyn_props = _remote_db->get_dynamic_global_properties();
        tx.set_reference_block(dyn_props.head_block_id);
        tx.set_expiration(dyn_props.time + fc::seconds(_tx_expiration_seconds));
        tx.signatures.clear();

        // idump((_keys));
        flat_set<public_key_type> available_keys;
        flat_map<public_key_type, fc::ecc::private_key> available_private_keys;
        for (const public_key_type& key : approving_key_set)
        {
            auto it = _keys.find(key);
            if (it != _keys.end())
            {
                fc::optional<fc::ecc::private_key> privkey = wif_to_key(it->second);
                FC_ASSERT(privkey.valid(), "Malformed private key in _keys");
                available_keys.insert(key);
                available_private_keys[key] = *privkey;
            }
        }

        auto get_account_from_lut = [&](const std::string& name) -> const account_api_obj& {
            auto it = approving_account_lut.find(name);
            FC_ASSERT(it != approving_account_lut.end());
            return it->second;
        };

        auto minimal_signing_keys
            = tx.minimize_required_signatures(_chain_id, available_keys,
                                              [&](const std::string& account_name) -> const authority& {
                                                  return (get_account_from_lut(account_name).active);
                                              },
                                              [&](const std::string& account_name) -> const authority& {
                                                  return (get_account_from_lut(account_name).owner);
                                              },
                                              [&](const std::string& account_name) -> const authority& {
                                                  return (get_account_from_lut(account_name).posting);
                                              },
                                              SCORUM_MAX_SIG_CHECK_DEPTH);

        for (const public_key_type& k : minimal_signing_keys)
        {
            auto it = available_private_keys.find(k);
            FC_ASSERT(it != available_private_keys.end());
            tx.sign(it->second, _chain_id);
        }

        if (broadcast)
        {
            try
            {
                auto result = _remote_net_broadcast->broadcast_transaction_synchronous(tx);
                annotated_signed_transaction rtrx(tx);
                rtrx.block_num = result.get_object()["block_num"].as_uint64();
                rtrx.transaction_num = result.get_object()["trx_num"].as_uint64();
                return rtrx;
            }
            catch (const fc::exception& e)
            {
                elog("Caught exception while broadcasting tx ${id}:  ${e}",
                     ("id", tx.id().str())("e", e.to_detail_string()));
                throw;
            }
        }
        return tx;
    }

    std::string print_atomicswap_secret2str(const std::string& secret) const
    {
        cli::formatter p;

        p.print_line();
        p.print_field("SECRET: ", secret);

        return p.str();
    }

    std::string print_atomicswap_contract2str(const atomicswap_contract_info_api_obj& rt) const
    {
        cli::formatter p;

        p.print_line();

        if (rt.contract_initiator)
        {
            p.print_raw("INITIATION CONTRACT");
        }
        else
        {
            p.print_raw("* PARTICIPATION CONTRACT");
        }

        p.print_line();
        p.print_field("From: ", rt.owner);
        p.print_field("To: ", rt.to);
        p.print_field("Amount: ", rt.amount);

        if (rt.deadline.sec_since_epoch())
        {
            p.print_field("Locktime: ", rt.deadline.to_iso_string());

            time_point_sec now = rt.deadline;
            now -= fc::time_point::now().sec_since_epoch();
            int h = now.sec_since_epoch() / 3600;
            int m = now.sec_since_epoch() % 3600 / 60;
            int s = now.sec_since_epoch() % 60;
            p.print_field("Locktime reached in: ", p.print_sequence2str(h, 'h', m, 'm', s, 's'));
        }

        if (!rt.secret.empty())
        {
            p.print_raw(print_atomicswap_secret2str(rt.secret));
        }

        p.print_line();
        p.print_field("Secret Hash: ", rt.secret_hash);

        if (!rt.metadata.empty())
        {
            p.print_line();
            p.print_field("Metadata: ", rt.metadata);
        }

        return p.str();
    }

    std::map<std::string, std::function<std::string(fc::variant, const fc::variants&)>> get_result_formatters() const
    {
        std::map<std::string, std::function<std::string(fc::variant, const fc::variants&)>> m;
        m["help"] = [](variant result, const fc::variants& a) { return result.get_string(); };

        m["gethelp"] = [](variant result, const fc::variants& a) { return result.get_string(); };

        m["list_my_accounts"] = [this](variant result, const fc::variants& a) {
            auto accounts = result.as<std::vector<account_api_obj>>();

            cli::formatter p;

            asset total_scorum(0, SCORUM_SYMBOL);
            asset total_vest(0, SP_SYMBOL);

            FC_ASSERT(p.create_table(16, 20, 10, 20));

            for (const auto& a : accounts)
            {
                total_scorum += a.balance;
                total_vest += a.scorumpower;
                p.print_cell(a.name);
                p.print_cell(a.balance);
                p.print_cell("");
                p.print_cell(a.scorumpower);
            }
            p.print_endl();
            p.print_line('-', accounts.empty());
            p.print_cell("TOTAL");
            p.print_cell(total_scorum);
            p.print_cell("");
            p.print_cell(total_vest);

            return p.str();
        };
        m["get_account_balance"] = [this](variant result, const fc::variants& a) -> std::string {
            auto rt = result.as<account_balance_info_api_obj>();

            cli::formatter p;

            FC_ASSERT(p.create_table(16, 20, 10, 20));

            p.print_line();
            p.print_cell("Scorums:");
            p.print_cell(rt.balance);
            p.print_cell("Vests:");
            p.print_cell(rt.scorumpower);

            return p.str();
        };

        auto account_history_formatter = [this](variant result, const fc::variants& a) {
            const auto& results = result.get_array();

            cli::formatter p;

            FC_ASSERT(p.create_table(5, 10, 15, 20, 50));

            p.print_cell("#");
            p.print_cell("BLOCK #");
            p.print_cell("TRX ID");
            p.print_cell("OPERATION");
            p.print_cell("DETAILS");
            p.print_endl();
            p.print_line('-', false);

            for (const auto& item : results)
            {
                p.print_cell(item.get_array()[0].as_string());
                const auto& op = item.get_array()[1].get_object();
                p.print_cell(op["block"].as_string());
                p.print_cell(op["trx_id"].as_string());
                const auto& opop = op["op"].get_array();
                p.print_cell(opop[0].as_string());
                p.print_cell(fc::json::to_string(opop[1]));
            }
            return p.str();
        };

        m["get_account_history"] = account_history_formatter;
        m["get_account_scr_to_scr_transfers"] = account_history_formatter;
        m["get_account_scr_to_sp_transfers"] = account_history_formatter;

        m["get_withdraw_routes"] = [this](variant result, const fc::variants& a) {
            auto routes = result.as<std::vector<withdraw_route>>();

            cli::formatter p;

            FC_ASSERT(p.create_table(20, 20, 8, 9));

            p.print_cell("From");
            p.print_cell("To");
            p.print_cell("Percent");
            p.print_cell("Auto-Vest");
            p.print_endl();
            p.print_line('=', false);

            for (auto r : routes)
            {
                p.print_cell(r.from_account);
                p.print_cell(r.to_account);
                std::stringstream tmp;
                tmp << std::setprecision(2) << std::fixed << double(r.percent) / 100;
                p.print_cell(tmp.str());
                p.print_cell((r.auto_vest ? "true" : "false"));
            }

            return p.str();
        };
        m["atomicswap_initiate"] = [this](variant result, const fc::variants& a) -> std::string {
            auto rt = result.as<atomicswap_contract_result_api_obj>();
            if (rt.empty())
            {
                return "";
            }
            else
            {
                return print_atomicswap_contract2str(rt.obj);
            }
        };
        m["atomicswap_participate"] = [this](variant result, const fc::variants& a) -> std::string {
            auto rt = result.as<atomicswap_contract_result_api_obj>();
            if (rt.empty())
            {
                return "";
            }
            else
            {
                return print_atomicswap_contract2str(rt.obj);
            }
        };
        m["atomicswap_auditcontract"] = [this](variant result, const fc::variants& a) -> std::string {
            auto rt = result.as<atomicswap_contract_info_api_obj>();
            if (rt.empty())
            {
                return "Nothing to audit.";
            }
            else
            {
                return print_atomicswap_contract2str(rt);
            }
        };
        m["atomicswap_extractsecret"] = [this](variant result, const fc::variants& a) -> std::string {
            auto secret = result.as<std::string>();
            if (secret.empty())
            {
                return "";
            }
            else
            {
                return print_atomicswap_secret2str(secret);
            }
        };

        return m;
    }

    void use_network_node_api()
    {
        if (_remote_net_node)
            return;
        try
        {
            _remote_net_node = _remote_api->get_api_by_name("network_node_api")->as<network_node_api>();
        }
        catch (const fc::exception& e)
        {
            elog("Couldn't get network_node_api");
            throw(e);
        }
    }

    void use_remote_account_by_key_api()
    {
        if (_remote_account_by_key_api.valid())
            return;

        try
        {
            _remote_account_by_key_api
                = _remote_api->get_api_by_name("account_by_key_api")->as<account_by_key::account_by_key_api>();
        }
        catch (const fc::exception& e)
        {
            elog("Couldn't get account_by_key_api");
            throw(e);
        }
    }

    void use_remote_account_history_api()
    {
        if (_remote_account_history_api.valid())
            return;

        try
        {
            _remote_account_history_api
                = _remote_api->get_api_by_name(API_ACCOUNT_HISTORY)->as<blockchain_history::account_history_api>();
        }
        catch (const fc::exception& e)
        {
            elog("Couldn't get account_history_api");
            throw(e);
        }
    }

    void use_remote_blockchain_history_api()
    {
        if (_remote_blockchain_history_api.valid())
            return;

        try
        {
            _remote_blockchain_history_api = _remote_api->get_api_by_name(API_BLOCKCHAIN_HISTORY)
                                                 ->as<blockchain_history::blockchain_history_api>();
        }
        catch (const fc::exception& e)
        {
            elog("Couldn't get blockchain_history_api");
            throw(e);
        }
    }

    void network_add_nodes(const std::vector<std::string>& nodes)
    {
        use_network_node_api();
        for (const std::string& node_address : nodes)
        {
            (*_remote_net_node)->add_node(fc::ip::endpoint::from_string(node_address));
        }
    }

    std::vector<variant> network_get_connected_peers()
    {
        use_network_node_api();
        const auto peers = (*_remote_net_node)->get_connected_peers();
        std::vector<variant> result;
        result.reserve(peers.size());
        for (const auto& peer : peers)
        {
            variant v;
            fc::to_variant(peer, v);
            result.push_back(v);
        }
        return result;
    }

    operation get_prototype_operation(const std::string& operation_name)
    {
        auto it = _prototype_ops.find(operation_name);
        if (it == _prototype_ops.end())
            FC_THROW("Unsupported operation: \"${operation_name}\"", ("operation_name", operation_name));
        return it->second;
    }

    std::string _wallet_filename;
    wallet_data _wallet;

    std::map<public_key_type, std::string> _keys;
    fc::sha512 _checksum;

    chain_id_type _chain_id;

    fc::api<login_api> _remote_api;
    fc::api<database_api> _remote_db;
    fc::api<chain_api> _chain_api;
    fc::api<network_broadcast_api> _remote_net_broadcast;
    optional<fc::api<network_node_api>> _remote_net_node;
    optional<fc::api<account_by_key::account_by_key_api>> _remote_account_by_key_api;
    optional<fc::api<blockchain_history::account_history_api>> _remote_account_history_api;
    optional<fc::api<blockchain_history::blockchain_history_api>> _remote_blockchain_history_api;

    uint32_t _tx_expiration_seconds = 30;

    flat_map<std::string, operation> _prototype_ops;

    static_variant_map _operation_which_map = create_static_variant_map<operation>();

#ifdef __unix__
    mode_t _old_umask;
#endif
    const std::string _wallet_filename_extension = ".wallet";
};

} // namespace detail

wallet_api::wallet_api(const wallet_data& initial_data, fc::api<login_api> rapi)
    : my(new detail::wallet_api_impl(*this, initial_data, rapi))
    , exit_func([]() { FC_ASSERT(false, "Operation valid only in console mode."); })
{
}

wallet_api::~wallet_api()
{
}

void wallet_api::set_exit_func(exit_func_type fn)
{
    exit_func = fn;
}

bool wallet_api::copy_wallet_file(const std::string& destination_filename)
{
    return my->copy_wallet_file(destination_filename);
}

optional<block_header> wallet_api::get_block_header(uint32_t num) const
{
    my->use_remote_blockchain_history_api();

    return (*my->_remote_blockchain_history_api)->get_block_header(num);
}

optional<signed_block_api_obj> wallet_api::get_block(uint32_t num) const
{
    my->use_remote_blockchain_history_api();

    return (*my->_remote_blockchain_history_api)->get_block(num);
}

std::map<uint32_t, block_header> wallet_api::get_block_headers_history(uint32_t num, uint32_t limit) const
{
    my->use_remote_blockchain_history_api();

    return (*my->_remote_blockchain_history_api)->get_block_headers_history(num, limit);
}

std::map<uint32_t, signed_block_api_obj> wallet_api::get_blocks_history(uint32_t num, uint32_t limit) const
{
    my->use_remote_blockchain_history_api();

    return (*my->_remote_blockchain_history_api)->get_blocks_history(num, limit);
}

std::map<uint32_t, applied_operation> wallet_api::get_ops_in_block(uint32_t block_num,
                                                                   applied_operation_type type_of_operation) const
{
    my->use_remote_blockchain_history_api();

    return (*my->_remote_blockchain_history_api)->get_ops_in_block(block_num, type_of_operation);
}

std::map<uint32_t, applied_operation>
wallet_api::get_ops_history(uint32_t from_op, uint32_t limit, applied_operation_type type_of_operation) const
{
    my->use_remote_blockchain_history_api();

    return (*my->_remote_blockchain_history_api)->get_ops_history(from_op, limit, type_of_operation);
}

std::vector<account_api_obj> wallet_api::list_my_accounts()
{
    FC_ASSERT(!is_locked(), "Wallet must be unlocked to list accounts");
    std::vector<account_api_obj> result;

    my->use_remote_account_by_key_api();

    std::vector<public_key_type> pub_keys;
    pub_keys.reserve(my->_keys.size());

    for (const auto& item : my->_keys)
        pub_keys.push_back(item.first);

    auto refs = (*my->_remote_account_by_key_api)->get_key_references(pub_keys);
    std::set<std::string> names;
    for (const auto& item : refs)
        for (const auto& name : item)
            names.insert(name);

    result.reserve(names.size());
    for (const auto& name : names)
        result.emplace_back(get_account(name));

    return result;
}

std::set<std::string> wallet_api::list_accounts(const std::string& lowerbound, uint32_t limit)
{
    return my->_remote_db->lookup_accounts(lowerbound, limit);
}

std::vector<account_name_type> wallet_api::get_active_witnesses() const
{
    return my->_remote_db->get_active_witnesses();
}

std::string wallet_api::serialize_transaction(const signed_transaction& tx) const
{
    return fc::to_hex(fc::raw::pack(tx));
}

std::string wallet_api::get_wallet_filename() const
{
    return my->get_wallet_filename();
}

account_api_obj wallet_api::get_account(const std::string& account_name) const
{
    return my->get_account(account_name);
}

account_balance_info_api_obj wallet_api::get_account_balance(const std::string& account_name) const
{
    return my->get_account(account_name);
}

bool wallet_api::import_key(const std::string& wif_key)
{
    FC_ASSERT(!is_locked());
    // backup wallet
    fc::optional<fc::ecc::private_key> optional_private_key = wif_to_key(wif_key);
    if (!optional_private_key)
        FC_THROW("Invalid private key");
    //   string shorthash = detail::pubkey_to_shorthash( optional_private_key->get_public_key() );
    //   copy_wallet_file( "before-import-key-" + shorthash );

    if (my->import_key(wif_key))
    {
        save_wallet_file();
        //     copy_wallet_file( "after-import-key-" + shorthash );
        return true;
    }
    return false;
}

std::string wallet_api::normalize_brain_key(const std::string& s) const
{
    return scorum::wallet::normalize_brain_key(s);
}

variant wallet_api::info()
{
    return my->info();
}

variant_object wallet_api::about() const
{
    return my->about();
}

std::set<account_name_type> wallet_api::list_witnesses(const std::string& lowerbound, uint32_t limit)
{
    return my->_remote_db->lookup_witness_accounts(lowerbound, limit);
}

optional<witness_api_obj> wallet_api::get_witness(const std::string& owner_account)
{
    return my->get_witness(owner_account);
}

annotated_signed_transaction wallet_api::set_voting_proxy(const std::string& account_to_modify,
                                                          const std::string& voting_account,
                                                          bool broadcast /* = false */)
{
    return my->set_voting_proxy(account_to_modify, voting_account, broadcast);
}

void wallet_api::set_wallet_filename(const std::string& wallet_filename)
{
    my->_wallet_filename = wallet_filename;
}

brain_key_info wallet_api::suggest_brain_key() const
{
    return scorum::wallet::suggest_brain_key();
}

annotated_signed_transaction wallet_api::sign_transaction(const signed_transaction& tx, bool broadcast /* = false */)
{
    try
    {
        return my->sign_transaction(tx, broadcast);
    }
    FC_CAPTURE_AND_RETHROW((tx))
}

operation wallet_api::get_prototype_operation(const std::string& operation_name)
{
    return my->get_prototype_operation(operation_name);
}

void wallet_api::network_add_nodes(const std::vector<std::string>& nodes)
{
    my->network_add_nodes(nodes);
}

std::vector<variant> wallet_api::network_get_connected_peers()
{
    return my->network_get_connected_peers();
}

std::string wallet_api::help() const
{
    std::vector<std::string> method_names = my->method_documentation.get_method_names();
    std::stringstream ss;
    for (const std::string method_name : method_names)
    {
        try
        {
            ss << my->method_documentation.get_brief_description(method_name);
        }
        catch (const fc::key_not_found_exception&)
        {
            ss << method_name << " (no help available)\n";
        }
    }
    return ss.str();
}

std::string wallet_api::gethelp(const std::string& method) const
{
    fc::api<wallet_api> tmp;
    std::stringstream ss;
    ss << "\n";

    std::string doxygenHelpString = my->method_documentation.get_detailed_description(method);
    if (!doxygenHelpString.empty())
        ss << doxygenHelpString;
    else
        ss << "No help defined for method " << method << "\n";

    return ss.str();
}

bool wallet_api::load_wallet_file(const std::string& wallet_filename)
{
    return my->load_wallet_file(wallet_filename);
}

void wallet_api::save_wallet_file(const std::string& wallet_filename)
{
    my->save_wallet_file(wallet_filename);
}

std::map<std::string, std::function<std::string(fc::variant, const fc::variants&)>>
wallet_api::get_result_formatters() const
{
    return my->get_result_formatters();
}

bool wallet_api::is_locked() const
{
    return my->is_locked();
}
bool wallet_api::is_new() const
{
    return my->_wallet.cipher_keys.size() == 0;
}

void wallet_api::encrypt_keys()
{
    my->encrypt_keys();
}

void wallet_api::lock()
{
    try
    {
        FC_ASSERT(!is_locked());
        encrypt_keys();
        for (auto key : my->_keys)
            key.second = key_to_wif(fc::ecc::private_key());
        my->_keys.clear();
        my->_checksum = fc::sha512();
        my->self.lock_changed(true);
    }
    FC_CAPTURE_AND_RETHROW()
}

void wallet_api::unlock(const std::string& password)
{
    try
    {
        FC_ASSERT(password.size() > 0);
        auto pw = fc::sha512::hash(password.c_str(), password.size());
        std::vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
        auto pk = fc::raw::unpack<plain_keys>(decrypted);
        FC_ASSERT(pk.checksum == pw);
        my->_keys = std::move(pk.keys);
        my->_checksum = pk.checksum;
        my->self.lock_changed(false);
    }
    FC_CAPTURE_AND_RETHROW()
}

void wallet_api::set_password(const std::string& password)
{
    if (!is_new())
        FC_ASSERT(!is_locked(), "The wallet must be unlocked before the password can be set");
    my->_checksum = fc::sha512::hash(password.c_str(), password.size());
    lock();
}

std::map<public_key_type, std::string> wallet_api::list_keys()
{
    FC_ASSERT(!is_locked());
    return my->_keys;
}

std::string wallet_api::get_private_key(const public_key_type& pubkey) const
{
    return key_to_wif(my->get_private_key(pubkey));
}

std::pair<public_key_type, std::string> wallet_api::get_private_key_from_password(const std::string& account,
                                                                                  const std::string& role,
                                                                                  const std::string& password) const
{
    auto seed = account + role + password;
    FC_ASSERT(seed.size());
    auto secret = fc::sha256::hash(seed.c_str(), seed.size());
    auto priv = fc::ecc::private_key::regenerate(secret);
    return std::make_pair(public_key_type(priv.get_public_key()), key_to_wif(priv));
}

/**
 * This method is used by faucets to create new accounts for other users which must
 * provide their desired keys. The resulting account may not be controllable by this
 * wallet.
 */
annotated_signed_transaction wallet_api::create_account_with_keys(const std::string& creator,
                                                                  const std::string& newname,
                                                                  const std::string& json_meta,
                                                                  const public_key_type& owner,
                                                                  const public_key_type& active,
                                                                  const public_key_type& posting,
                                                                  const public_key_type& memo,
                                                                  bool broadcast) const
{
    try
    {
        FC_ASSERT(!is_locked());
        account_create_operation op;
        op.creator = creator;
        op.new_account_name = newname;
        op.owner = authority(1, owner, 1);
        op.active = authority(1, active, 1);
        op.posting = authority(1, posting, 1);
        op.memo_key = memo;
        op.json_metadata = json_meta;
        op.fee = my->_chain_api->get_chain_properties().median_chain_props.account_creation_fee
            * SCORUM_CREATE_ACCOUNT_WITH_SCORUM_MODIFIER;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction(tx, broadcast);
    }
    FC_CAPTURE_AND_RETHROW((creator)(newname)(json_meta)(owner)(active)(posting)(memo)(broadcast))
}

/**
 * This method is used by faucets to create new accounts for other users which must
 * provide their desired keys. The resulting account may not be controllable by this
 * wallet.
 */
annotated_signed_transaction wallet_api::create_account_with_keys_delegated(const std::string& creator,
                                                                            const asset& scorum_fee,
                                                                            const asset& delegated_scorumpower,
                                                                            const std::string& newname,
                                                                            const std::string& json_meta,
                                                                            const public_key_type& owner,
                                                                            const public_key_type& active,
                                                                            const public_key_type& posting,
                                                                            const public_key_type& memo,
                                                                            bool broadcast) const
{
    try
    {
        FC_ASSERT(!is_locked());
        account_create_with_delegation_operation op;
        op.creator = creator;
        op.new_account_name = newname;
        op.owner = authority(1, owner, 1);
        op.active = authority(1, active, 1);
        op.posting = authority(1, posting, 1);
        op.memo_key = memo;
        op.json_metadata = json_meta;
        op.fee = scorum_fee;
        op.delegation = delegated_scorumpower;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction(tx, broadcast);
    }
    FC_CAPTURE_AND_RETHROW((creator)(newname)(json_meta)(owner)(active)(posting)(memo)(broadcast))
}

annotated_signed_transaction wallet_api::create_account_by_committee(const std::string& creator,
                                                                     const std::string& newname,
                                                                     const std::string& json_meta,
                                                                     const public_key_type& owner,
                                                                     const public_key_type& active,
                                                                     const public_key_type& posting,
                                                                     const public_key_type& memo,
                                                                     bool broadcast) const
{
    try
    {
        FC_ASSERT(!is_locked());
        account_create_by_committee_operation op;
        op.creator = creator;
        op.new_account_name = newname;
        op.owner = authority(1, owner, 1);
        op.active = authority(1, active, 1);
        op.posting = authority(1, posting, 1);
        op.memo_key = memo;
        op.json_metadata = json_meta;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction(tx, broadcast);
    }
    FC_CAPTURE_AND_RETHROW((creator)(newname)(json_meta)(owner)(active)(posting)(memo)(broadcast))
}

annotated_signed_transaction wallet_api::request_account_recovery(const std::string& recovery_account,
                                                                  const std::string& account_to_recover,
                                                                  const authority& new_authority,
                                                                  bool broadcast)
{
    FC_ASSERT(!is_locked());
    request_account_recovery_operation op;
    op.recovery_account = recovery_account;
    op.account_to_recover = account_to_recover;
    op.new_owner_authority = new_authority;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::recover_account(const std::string& account_to_recover,
                                                         const authority& recent_authority,
                                                         const authority& new_authority,
                                                         bool broadcast)
{
    FC_ASSERT(!is_locked());

    recover_account_operation op;
    op.account_to_recover = account_to_recover;
    op.new_owner_authority = new_authority;
    op.recent_owner_authority = recent_authority;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction
wallet_api::change_recovery_account(const std::string& owner, const std::string& new_recovery_account, bool broadcast)
{
    FC_ASSERT(!is_locked());

    change_recovery_account_operation op;
    op.account_to_recover = owner;
    op.new_recovery_account = new_recovery_account;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

std::vector<owner_authority_history_api_obj> wallet_api::get_owner_history(const std::string& account) const
{
    return my->_remote_db->get_owner_history(account);
}

annotated_signed_transaction wallet_api::update_account(const std::string& account_name,
                                                        const std::string& json_meta,
                                                        const public_key_type& owner,
                                                        const public_key_type& active,
                                                        const public_key_type& posting,
                                                        const public_key_type& memo,
                                                        bool broadcast) const
{
    try
    {
        FC_ASSERT(!is_locked());

        account_update_operation op;
        op.account = account_name;
        op.owner = authority(1, owner, 1);
        op.active = authority(1, active, 1);
        op.posting = authority(1, posting, 1);
        op.memo_key = memo;
        op.json_metadata = json_meta;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction(tx, broadcast);
    }
    FC_CAPTURE_AND_RETHROW((account_name)(json_meta)(owner)(active)(posting)(memo)(broadcast))
}

annotated_signed_transaction wallet_api::update_account_auth_key(const std::string& account_name,
                                                                 const authority_type& type,
                                                                 const public_key_type& key,
                                                                 authority_weight_type weight,
                                                                 bool broadcast)
{
    FC_ASSERT(!is_locked());

    auto accounts = my->_remote_db->get_accounts({ account_name });
    FC_ASSERT(accounts.size() == 1, "Account does not exist");
    FC_ASSERT(account_name == accounts[0].name, "Account name doesn't match?");

    account_update_operation op;
    op.account = account_name;
    op.memo_key = accounts[0].memo_key;
    op.json_metadata = accounts[0].json_metadata;

    authority new_auth;

    switch (type)
    {
    case (owner):
        new_auth = accounts[0].owner;
        break;
    case (active):
        new_auth = accounts[0].active;
        break;
    case (posting):
        new_auth = accounts[0].posting;
        break;
    }

    if (weight == 0) // Remove the key
    {
        new_auth.key_auths.erase(key);
    }
    else
    {
        new_auth.add_authority(key, weight);
    }

    if (new_auth.is_impossible())
    {
        if (type == owner)
        {
            FC_ASSERT(false, "Owner authority change would render account irrecoverable.");
        }

        wlog("Authority is now impossible.");
    }

    switch (type)
    {
    case (owner):
        op.owner = new_auth;
        break;
    case (active):
        op.active = new_auth;
        break;
    case (posting):
        op.posting = new_auth;
        break;
    }

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::update_account_auth_account(const std::string& account_name,
                                                                     authority_type type,
                                                                     const std::string& auth_account,
                                                                     authority_weight_type weight,
                                                                     bool broadcast)
{
    FC_ASSERT(!is_locked());

    auto accounts = my->_remote_db->get_accounts({ account_name });
    FC_ASSERT(accounts.size() == 1, "Account does not exist");
    FC_ASSERT(account_name == accounts[0].name, "Account name doesn't match?");

    account_update_operation op;
    op.account = account_name;
    op.memo_key = accounts[0].memo_key;
    op.json_metadata = accounts[0].json_metadata;

    authority new_auth;

    switch (type)
    {
    case (owner):
        new_auth = accounts[0].owner;
        break;
    case (active):
        new_auth = accounts[0].active;
        break;
    case (posting):
        new_auth = accounts[0].posting;
        break;
    }

    if (weight == 0) // Remove the key
    {
        new_auth.account_auths.erase(auth_account);
    }
    else
    {
        new_auth.add_authority(auth_account, weight);
    }

    if (new_auth.is_impossible())
    {
        if (type == owner)
        {
            FC_ASSERT(false, "Owner authority change would render account irrecoverable.");
        }

        wlog("Authority is now impossible.");
    }

    switch (type)
    {
    case (owner):
        op.owner = new_auth;
        break;
    case (active):
        op.active = new_auth;
        break;
    case (posting):
        op.posting = new_auth;
        break;
    }

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::update_account_auth_threshold(const std::string& account_name,
                                                                       authority_type type,
                                                                       uint32_t threshold,
                                                                       bool broadcast)
{
    FC_ASSERT(!is_locked());

    auto accounts = my->_remote_db->get_accounts({ account_name });
    FC_ASSERT(accounts.size() == 1, "Account does not exist");
    FC_ASSERT(account_name == accounts[0].name, "Account name doesn't match?");
    FC_ASSERT(threshold != 0, "Authority is implicitly satisfied");

    account_update_operation op;
    op.account = account_name;
    op.memo_key = accounts[0].memo_key;
    op.json_metadata = accounts[0].json_metadata;

    authority new_auth;

    switch (type)
    {
    case (owner):
        new_auth = accounts[0].owner;
        break;
    case (active):
        new_auth = accounts[0].active;
        break;
    case (posting):
        new_auth = accounts[0].posting;
        break;
    }

    new_auth.weight_threshold = threshold;

    if (new_auth.is_impossible())
    {
        if (type == owner)
        {
            FC_ASSERT(false, "Owner authority change would render account irrecoverable.");
        }

        wlog("Authority is now impossible.");
    }

    switch (type)
    {
    case (owner):
        op.owner = new_auth;
        break;
    case (active):
        op.active = new_auth;
        break;
    case (posting):
        op.posting = new_auth;
        break;
    }

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction
wallet_api::update_account_meta(const std::string& account_name, const std::string& json_meta, bool broadcast)
{
    FC_ASSERT(!is_locked());

    auto accounts = my->_remote_db->get_accounts({ account_name });
    FC_ASSERT(accounts.size() == 1, "Account does not exist");
    FC_ASSERT(account_name == accounts[0].name, "Account name doesn't match?");

    account_update_operation op;
    op.account = account_name;
    op.memo_key = accounts[0].memo_key;
    op.json_metadata = json_meta;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction
wallet_api::update_account_memo_key(const std::string& account_name, const public_key_type& key, bool broadcast)
{
    FC_ASSERT(!is_locked());

    auto accounts = my->_remote_db->get_accounts({ account_name });
    FC_ASSERT(accounts.size() == 1, "Account does not exist");
    FC_ASSERT(account_name == accounts[0].name, "Account name doesn't match?");

    account_update_operation op;
    op.account = account_name;
    op.memo_key = key;
    op.json_metadata = accounts[0].json_metadata;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::delegate_scorumpower(const std::string& delegator,
                                                              const std::string& delegatee,
                                                              const asset& scorumpower,
                                                              bool broadcast)
{
    FC_ASSERT(!is_locked());

    auto accounts = my->_remote_db->get_accounts({ delegator, delegatee });
    FC_ASSERT(accounts.size() == 2, "One or more of the accounts specified do not exist.");
    FC_ASSERT(delegator == accounts[0].name, "Delegator account is not right?");
    FC_ASSERT(delegatee == accounts[1].name, "Delegator account is not right?");

    delegate_scorumpower_operation op;
    op.delegator = delegator;
    op.delegatee = delegatee;
    op.scorumpower = scorumpower;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

/**
 *  This method will genrate new owner, active, and memo keys for the new account which
 *  will be controlable by this wallet.
 */
annotated_signed_transaction wallet_api::create_account(const std::string& creator,
                                                        const std::string& newname,
                                                        const std::string& json_meta,
                                                        bool broadcast)
{
    try
    {
        FC_ASSERT(!is_locked());
        auto owner = suggest_brain_key();
        auto active = suggest_brain_key();
        auto posting = suggest_brain_key();
        auto memo = suggest_brain_key();
        import_key(owner.wif_priv_key);
        import_key(active.wif_priv_key);
        import_key(posting.wif_priv_key);
        import_key(memo.wif_priv_key);
        return create_account_with_keys(creator, newname, json_meta, owner.pub_key, active.pub_key, posting.pub_key,
                                        memo.pub_key, broadcast);
    }
    FC_CAPTURE_AND_RETHROW((creator)(newname)(json_meta))
}

/**
 *  This method will genrate new owner, active, and memo keys for the new account which
 *  will be controlable by this wallet.
 */
annotated_signed_transaction wallet_api::create_account_delegated(const std::string& creator,
                                                                  const asset& scorum_fee,
                                                                  const asset& delegated_scorumpower,
                                                                  const std::string& newname,
                                                                  const std::string& json_meta,
                                                                  bool broadcast)
{
    try
    {
        FC_ASSERT(!is_locked());
        auto owner = suggest_brain_key();
        auto active = suggest_brain_key();
        auto posting = suggest_brain_key();
        auto memo = suggest_brain_key();
        import_key(owner.wif_priv_key);
        import_key(active.wif_priv_key);
        import_key(posting.wif_priv_key);
        import_key(memo.wif_priv_key);
        return create_account_with_keys_delegated(creator, scorum_fee, delegated_scorumpower, newname, json_meta,
                                                  owner.pub_key, active.pub_key, posting.pub_key, memo.pub_key,
                                                  broadcast);
    }
    FC_CAPTURE_AND_RETHROW((creator)(newname)(json_meta))
}

annotated_signed_transaction wallet_api::update_witness(const std::string& witness_account_name,
                                                        const std::string& url,
                                                        const public_key_type& block_signing_key,
                                                        const chain_properties& props,
                                                        bool broadcast)
{
    FC_ASSERT(!is_locked());

    witness_update_operation op;

    fc::optional<witness_api_obj> wit = my->_remote_db->get_witness_by_account(witness_account_name);
    if (!wit.valid())
    {
        op.url = url;
    }
    else
    {
        FC_ASSERT(wit->owner == witness_account_name);
        if (url != "")
            op.url = url;
        else
            op.url = wit->url;
    }
    op.owner = witness_account_name;
    op.block_signing_key = block_signing_key;
    op.proposed_chain_props = props;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::vote_for_witness(const std::string& voting_account,
                                                          const std::string& witness_to_vote_for,
                                                          bool approve,
                                                          bool broadcast)
{
    try
    {
        FC_ASSERT(!is_locked());
        account_witness_vote_operation op;
        op.account = voting_account;
        op.witness = witness_to_vote_for;
        op.approve = approve;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction(tx, broadcast);
    }
    FC_CAPTURE_AND_RETHROW((voting_account)(witness_to_vote_for)(approve)(broadcast))
}

void wallet_api::check_memo(const std::string& memo, const account_api_obj& account) const
{
    std::vector<public_key_type> keys;

    try
    {
        // Check if memo is a private key
        keys.push_back(fc::ecc::extended_private_key::from_base58(memo).get_public_key());
    }
    catch (fc::parse_error_exception&)
    {
    }
    catch (fc::assert_exception&)
    {
    }

    // Get possible keys if memo was an account password
    std::string owner_seed = account.name + "owner" + memo;
    auto owner_secret = fc::sha256::hash(owner_seed.c_str(), owner_seed.size());
    keys.push_back(fc::ecc::private_key::regenerate(owner_secret).get_public_key());

    std::string active_seed = account.name + "active" + memo;
    auto active_secret = fc::sha256::hash(active_seed.c_str(), active_seed.size());
    keys.push_back(fc::ecc::private_key::regenerate(active_secret).get_public_key());

    std::string posting_seed = account.name + "posting" + memo;
    auto posting_secret = fc::sha256::hash(posting_seed.c_str(), posting_seed.size());
    keys.push_back(fc::ecc::private_key::regenerate(posting_secret).get_public_key());

    // Check keys against public keys in authorites
    for (auto& key_weight_pair : account.owner.key_auths)
    {
        for (auto& key : keys)
            FC_ASSERT(key_weight_pair.first != key,
                      "Detected private owner key in memo field. Cancelling transaction.");
    }

    for (auto& key_weight_pair : account.active.key_auths)
    {
        for (auto& key : keys)
            FC_ASSERT(key_weight_pair.first != key,
                      "Detected private active key in memo field. Cancelling transaction.");
    }

    for (auto& key_weight_pair : account.posting.key_auths)
    {
        for (auto& key : keys)
            FC_ASSERT(key_weight_pair.first != key,
                      "Detected private posting key in memo field. Cancelling transaction.");
    }

    const auto& memo_key = account.memo_key;
    for (auto& key : keys)
        FC_ASSERT(memo_key != key, "Detected private memo key in memo field. Cancelling transaction.");

    // Check against imported keys
    for (auto& key_pair : my->_keys)
    {
        for (auto& key : keys)
            FC_ASSERT(key != key_pair.first, "Detected imported private key in memo field. Cancelling trasanction.");
    }
}

std::string wallet_api::get_encrypted_memo(const std::string& from, const std::string& to, const std::string& memo)
{

    if (memo.size() > 0 && memo[0] == '#')
    {
        memo_data m;

        auto from_account = get_account(from);
        auto to_account = get_account(to);

        m.from = from_account.memo_key;
        m.to = to_account.memo_key;
        m.nonce = fc::time_point::now().time_since_epoch().count();

        auto from_priv = my->get_private_key(m.from);
        auto shared_secret = from_priv.get_shared_secret(m.to);

        fc::sha512::encoder enc;
        fc::raw::pack(enc, m.nonce);
        fc::raw::pack(enc, shared_secret);
        auto encrypt_key = enc.result();

        m.encrypted = fc::aes_encrypt(encrypt_key, fc::raw::pack(memo.substr(1)));
        m.check = fc::sha256::hash(encrypt_key)._hash[0];
        return m;
    }
    else
    {
        return memo;
    }
}

annotated_signed_transaction wallet_api::transfer(
    const std::string& from, const std::string& to, const asset& amount, const std::string& memo, bool broadcast)
{
    try
    {
        FC_ASSERT(!is_locked());
        check_memo(memo, get_account(from));
        transfer_operation op;
        op.from = from;
        op.to = to;
        op.amount = amount;

        op.memo = get_encrypted_memo(from, to, memo);

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.validate();

        return my->sign_transaction(tx, broadcast);
    }
    FC_CAPTURE_AND_RETHROW((from)(to)(amount)(memo)(broadcast))
}

annotated_signed_transaction wallet_api::escrow_transfer(const std::string& from,
                                                         const std::string& to,
                                                         const std::string& agent,
                                                         uint32_t escrow_id,
                                                         const asset& scorum_amount,
                                                         const asset& fee,
                                                         time_point_sec ratification_deadline,
                                                         time_point_sec escrow_expiration,
                                                         const std::string& json_meta,
                                                         bool broadcast)
{
    FC_ASSERT(!is_locked());
    escrow_transfer_operation op;
    op.from = from;
    op.to = to;
    op.agent = agent;
    op.escrow_id = escrow_id;
    op.scorum_amount = scorum_amount;
    op.fee = fee;
    op.ratification_deadline = ratification_deadline;
    op.escrow_expiration = escrow_expiration;
    op.json_meta = json_meta;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::escrow_approve(const std::string& from,
                                                        const std::string& to,
                                                        const std::string& agent,
                                                        const std::string& who,
                                                        uint32_t escrow_id,
                                                        bool approve,
                                                        bool broadcast)
{
    FC_ASSERT(!is_locked());
    escrow_approve_operation op;
    op.from = from;
    op.to = to;
    op.agent = agent;
    op.who = who;
    op.escrow_id = escrow_id;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::escrow_dispute(const std::string& from,
                                                        const std::string& to,
                                                        const std::string& agent,
                                                        const std::string& who,
                                                        uint32_t escrow_id,
                                                        bool broadcast)
{
    FC_ASSERT(!is_locked());
    escrow_dispute_operation op;
    op.from = from;
    op.to = to;
    op.agent = agent;
    op.who = who;
    op.escrow_id = escrow_id;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::escrow_release(const std::string& from,
                                                        const std::string& to,
                                                        const std::string& agent,
                                                        const std::string& who,
                                                        const std::string& receiver,
                                                        uint32_t escrow_id,
                                                        const asset& scorum_amount,
                                                        bool broadcast)
{
    FC_ASSERT(!is_locked());
    escrow_release_operation op;
    op.from = from;
    op.to = to;
    op.agent = agent;
    op.who = who;
    op.receiver = receiver;
    op.escrow_id = escrow_id;
    op.scorum_amount = scorum_amount;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();
    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction
wallet_api::transfer_to_scorumpower(const std::string& from, const std::string& to, const asset& amount, bool broadcast)
{
    FC_ASSERT(!is_locked());
    transfer_to_scorumpower_operation op;
    op.from = from;
    op.to = (to == from ? "" : to);
    op.amount = amount;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction
wallet_api::withdraw_scorumpower(const std::string& from, const asset& scorumpower, bool broadcast)
{
    FC_ASSERT(!is_locked());
    withdraw_scorumpower_operation op;
    op.account = from;
    op.scorumpower = scorumpower;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::set_withdraw_scorumpower_route(
    const std::string& from, const std::string& to, uint16_t percent, bool auto_vest, bool broadcast)
{
    FC_ASSERT(!is_locked());
    set_withdraw_scorumpower_route_to_account_operation op;
    op.from_account = from;
    op.to_account = to;
    op.percent = percent;
    op.auto_vest = auto_vest;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

std::string wallet_api::decrypt_memo(const std::string& encrypted_memo)
{
    if (is_locked())
        return encrypted_memo;

    if (encrypted_memo.size() && encrypted_memo[0] == '#')
    {
        auto m = memo_data::from_string(encrypted_memo);
        if (m)
        {
            fc::sha512 shared_secret;
            auto from_key = my->try_get_private_key(m->from);
            if (!from_key)
            {
                auto to_key = my->try_get_private_key(m->to);
                if (!to_key)
                    return encrypted_memo;
                shared_secret = to_key->get_shared_secret(m->from);
            }
            else
            {
                shared_secret = from_key->get_shared_secret(m->to);
            }
            fc::sha512::encoder enc;
            fc::raw::pack(enc, m->nonce);
            fc::raw::pack(enc, shared_secret);
            auto encryption_key = enc.result();

            uint32_t check = fc::sha256::hash(encryption_key)._hash[0];
            if (check != m->check)
                return encrypted_memo;

            try
            {
                std::vector<char> decrypted = fc::aes_decrypt(encryption_key, m->encrypted);
                return fc::raw::unpack<std::string>(decrypted);
            }
            catch (...)
            {
            }
        }
    }
    return encrypted_memo;
}

annotated_signed_transaction wallet_api::decline_voting_rights(const std::string& account, bool decline, bool broadcast)
{
    FC_ASSERT(!is_locked());
    decline_voting_rights_operation op;
    op.account = account;
    op.decline = decline;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

std::map<uint32_t, applied_operation>
wallet_api::get_account_history(const std::string& account, uint64_t from, uint32_t limit)
{
    FC_ASSERT(!is_locked(), "Wallet must be unlocked to get account history");

    std::map<uint32_t, applied_operation> result;

    my->use_remote_account_history_api();

    result = (*my->_remote_account_history_api)->get_account_history(account, from, limit);

    for (auto& item : result)
    {
        if (item.second.op.which() == operation::tag<transfer_operation>::value)
        {
            auto& top = item.second.op.get<transfer_operation>();
            top.memo = decrypt_memo(top.memo);
        }
    }

    return result;
}

std::map<uint32_t, applied_operation>
wallet_api::get_account_scr_to_scr_transfers(const std::string& account, uint64_t from, uint32_t limit)
{
    FC_ASSERT(!is_locked(), "Wallet must be unlocked to get account history");

    std::map<uint32_t, applied_operation> result;

    my->use_remote_account_history_api();

    result = (*my->_remote_account_history_api)->get_account_scr_to_scr_transfers(account, from, limit);

    for (auto& item : result)
    {
        if (item.second.op.which() == operation::tag<transfer_operation>::value)
        {
            auto& top = item.second.op.get<transfer_operation>();
            top.memo = decrypt_memo(top.memo);
        }
    }

    return result;
}

std::map<uint32_t, applied_operation>
wallet_api::get_account_scr_to_sp_transfers(const std::string& account, uint64_t from, uint32_t limit)
{
    FC_ASSERT(!is_locked(), "Wallet must be unlocked to get account history");

    std::map<uint32_t, applied_operation> result;

    my->use_remote_account_history_api();

    result = (*my->_remote_account_history_api)->get_account_scr_to_sp_transfers(account, from, limit);

    for (auto& item : result)
    {
        if (item.second.op.which() == operation::tag<transfer_operation>::value)
        {
            auto& top = item.second.op.get<transfer_operation>();
            top.memo = decrypt_memo(top.memo);
        }
    }

    return result;
}

std::vector<withdraw_route> wallet_api::get_withdraw_routes(const std::string& account, withdraw_route_type type) const
{
    return my->_remote_db->get_withdraw_routes(account, type);
}

annotated_signed_transaction wallet_api::post_comment(const std::string& author,
                                                      const std::string& permlink,
                                                      const std::string& parent_author,
                                                      const std::string& parent_permlink,
                                                      const std::string& title,
                                                      const std::string& body,
                                                      const std::string& json,
                                                      bool broadcast)
{
    FC_ASSERT(!is_locked());
    comment_operation op;
    op.parent_author = parent_author;
    op.parent_permlink = parent_permlink;
    op.author = author;
    op.permlink = permlink;
    op.title = title;
    op.body = body;
    op.json_metadata = json;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::vote(
    const std::string& voter, const std::string& author, const std::string& permlink, int16_t weight, bool broadcast)
{
    FC_ASSERT(!is_locked());
    FC_ASSERT(abs(weight) <= 100, "Weight must be between -100 and 100 and not 0");

    vote_operation op;
    op.voter = voter;
    op.author = author;
    op.permlink = permlink;
    op.weight = weight;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

void wallet_api::set_transaction_expiration(uint32_t seconds)
{
    my->set_transaction_expiration(seconds);
}

annotated_signed_transaction
wallet_api::challenge(const std::string& challenger, const std::string& challenged, bool broadcast)
{
    // SCORUM: TODO: remove whole method
    FC_ASSERT(false, "Challenge is disabled");
    /*
    FC_ASSERT( !is_locked() );

    challenge_authority_operation op;
    op.challenger = challenger;
    op.challenged = challenged;
    op.require_owner = false;

    signed_transaction tx;
    tx.operations.push_back( op );
    tx.validate();

    return my->sign_transaction( tx, broadcast );
    */
}

annotated_signed_transaction wallet_api::prove(const std::string& challenged, bool broadcast)
{
    FC_ASSERT(!is_locked());

    prove_authority_operation op;
    op.challenged = challenged;
    op.require_owner = false;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::get_transaction(transaction_id_type id) const
{
    my->use_remote_blockchain_history_api();

    return (*my->_remote_blockchain_history_api)->get_transaction(id);
}

std::vector<budget_api_obj> wallet_api::list_my_budgets()
{
    FC_ASSERT(!is_locked());

    my->use_remote_account_by_key_api();

    std::vector<public_key_type> pub_keys;
    pub_keys.reserve(my->_keys.size());

    for (const auto& item : my->_keys)
        pub_keys.push_back(item.first);

    auto refs = (*my->_remote_account_by_key_api)->get_key_references(pub_keys);
    std::set<std::string> names;
    for (const auto& item : refs)
        for (const auto& name : item)
            names.insert(name);

    return my->_remote_db->get_budgets(names);
}

std::set<std::string> wallet_api::list_budget_owners(const std::string& lowerbound, uint32_t limit)
{
    return my->_remote_db->lookup_budget_owners(lowerbound, limit);
}

std::vector<budget_api_obj> wallet_api::get_budgets(const std::string& account_name)
{
    validate_account_name(account_name);

    std::vector<budget_api_obj> result;

    result = my->_remote_db->get_budgets({ account_name });

    return result;
}

annotated_signed_transaction wallet_api::create_budget(const std::string& budget_owner,
                                                       const std::string& content_permlink,
                                                       const asset& balance,
                                                       const time_point_sec deadline,
                                                       const bool broadcast)
{
    FC_ASSERT(!is_locked());

    create_budget_operation op;

    op.owner = budget_owner;
    op.content_permlink = content_permlink;
    op.balance = balance;
    op.deadline = deadline;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction
wallet_api::close_budget(const int64_t id, const std::string& budget_owner, const bool broadcast)
{
    FC_ASSERT(!is_locked());

    close_budget_operation op;

    op.budget_id = id;
    op.owner = budget_owner;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

template <typename T, typename C>
signed_transaction proposal(const std::string& initiator, uint32_t lifetime_sec, C&& constructor)
{
    T operation;
    constructor(operation);

    proposal_create_operation op;
    op.creator = initiator;
    op.operation = operation;
    op.lifetime_sec = lifetime_sec;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return tx;
}

annotated_signed_transaction
wallet_api::vote_for_committee_proposal(const std::string& voting_account, int64_t proposal_id, bool broadcast)
{
    proposal_vote_operation op;
    op.voting_account = voting_account;
    op.proposal_id = proposal_id;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::registration_committee_add_member(const std::string& initiator,
                                                                           const std::string& invitee,
                                                                           uint32_t lifetime_sec,
                                                                           bool broadcast)
{
    using operation_type = registration_committee_add_member_operation;

    signed_transaction tx
        = proposal<operation_type>(initiator, lifetime_sec, [&](operation_type& o) { o.account_name = invitee; });

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::registration_committee_exclude_member(const std::string& initiator,
                                                                               const std::string& dropout,
                                                                               uint32_t lifetime_sec,
                                                                               bool broadcast)
{
    using operation_type = registration_committee_exclude_member_operation;

    signed_transaction tx
        = proposal<operation_type>(initiator, lifetime_sec, [&](operation_type& o) { o.account_name = dropout; });

    return my->sign_transaction(tx, broadcast);
}

std::set<account_name_type> wallet_api::list_registration_committee(const std::string& lowerbound, uint32_t limit)
{
    return my->_remote_db->lookup_registration_committee_members(lowerbound, limit);
}

registration_committee_api_obj wallet_api::get_registration_committee()
{
    return my->_remote_db->get_registration_committee();
}

std::vector<proposal_api_obj> wallet_api::list_proposals()
{
    return my->_remote_db->lookup_proposals();
}

annotated_signed_transaction wallet_api::registration_committee_change_add_member_quorum(const std::string& initiator,
                                                                                         uint64_t quorum_percent,
                                                                                         uint32_t lifetime_sec,
                                                                                         bool broadcast)
{
    using operation_type = registration_committee_change_quorum_operation;

    signed_transaction tx = proposal<operation_type>(initiator, lifetime_sec, [&](operation_type& o) {
        o.quorum = quorum_percent;
        o.committee_quorum = add_member_quorum;
    });

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::registration_committee_change_exclude_member_quorum(
    const std::string& initiator, uint64_t quorum_percent, uint32_t lifetime_sec, bool broadcast)
{
    using operation_type = registration_committee_change_quorum_operation;

    signed_transaction tx = proposal<operation_type>(initiator, lifetime_sec, [&](operation_type& o) {
        o.quorum = quorum_percent;
        o.committee_quorum = exclude_member_quorum;
    });

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::registration_committee_change_base_quorum(const std::string& initiator,
                                                                                   uint64_t quorum_percent,
                                                                                   uint32_t lifetime_sec,
                                                                                   bool broadcast)
{
    using operation_type = registration_committee_change_quorum_operation;

    signed_transaction tx = proposal<operation_type>(initiator, lifetime_sec, [&](operation_type& o) {
        o.quorum = quorum_percent;
        o.committee_quorum = base_quorum;
    });

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::development_committee_add_member(const std::string& initiator,
                                                                          const std::string& invitee,
                                                                          uint32_t lifetime_sec,
                                                                          bool broadcast)
{
    using operation_type = development_committee_add_member_operation;

    signed_transaction tx
        = proposal<operation_type>(initiator, lifetime_sec, [&](operation_type& o) { o.account_name = invitee; });

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::development_committee_exclude_member(const std::string& initiator,
                                                                              const std::string& dropout,
                                                                              uint32_t lifetime_sec,
                                                                              bool broadcast)
{
    using operation_type = development_committee_exclude_member_operation;

    signed_transaction tx
        = proposal<operation_type>(initiator, lifetime_sec, [&](operation_type& o) { o.account_name = dropout; });

    return my->sign_transaction(tx, broadcast);
}

std::set<account_name_type> wallet_api::list_development_committee(const std::string& lowerbound, uint32_t limit)
{
    return my->_remote_db->lookup_development_committee_members(lowerbound, limit);
}

annotated_signed_transaction wallet_api::development_committee_change_add_member_quorum(const std::string& initiator,
                                                                                        uint64_t quorum_percent,
                                                                                        uint32_t lifetime_sec,
                                                                                        bool broadcast)
{
    using operation_type = development_committee_change_quorum_operation;

    signed_transaction tx = proposal<operation_type>(initiator, lifetime_sec, [&](operation_type& o) {
        o.quorum = quorum_percent;
        o.committee_quorum = add_member_quorum;
    });

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::development_committee_change_exclude_member_quorum(
    const std::string& initiator, uint64_t quorum_percent, uint32_t lifetime_sec, bool broadcast)
{
    using operation_type = development_committee_change_quorum_operation;

    signed_transaction tx = proposal<operation_type>(initiator, lifetime_sec, [&](operation_type& o) {
        o.quorum = quorum_percent;
        o.committee_quorum = exclude_member_quorum;
    });

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::development_committee_change_base_quorum(const std::string& initiator,
                                                                                  uint64_t quorum_percent,
                                                                                  uint32_t lifetime_sec,
                                                                                  bool broadcast)
{
    using operation_type = development_committee_change_quorum_operation;

    signed_transaction tx = proposal<operation_type>(initiator, lifetime_sec, [&](operation_type& o) {
        o.quorum = quorum_percent;
        o.committee_quorum = base_quorum;
    });

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::development_committee_change_transfer_quorum(const std::string& initiator,
                                                                                      uint64_t quorum_percent,
                                                                                      uint32_t lifetime_sec,
                                                                                      bool broadcast)
{
    using operation_type = development_committee_change_quorum_operation;

    signed_transaction tx = proposal<operation_type>(initiator, lifetime_sec, [&](operation_type& o) {
        o.quorum = quorum_percent;
        o.committee_quorum = transfer_quorum;
    });

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::development_pool_transfer(
    const std::string& initiator, const std::string& to_account, asset amount, uint32_t lifetime_sec, bool broadcast)
{
    using operation_type = development_committee_transfer_operation;

    signed_transaction tx = proposal<operation_type>(initiator, lifetime_sec, [&](operation_type& o) {
        o.to_account = to_account;
        o.amount = amount;
    });

    return my->sign_transaction(tx, broadcast);
}

annotated_signed_transaction wallet_api::development_pool_withdraw_vesting(const std::string& initiator,
                                                                           asset amount,
                                                                           uint32_t lifetime_sec,
                                                                           bool broadcast)
{
    using operation_type = development_committee_withdraw_vesting_operation;

    signed_transaction tx
        = proposal<operation_type>(initiator, lifetime_sec, [&](operation_type& o) { o.vesting_shares = amount; });

    return my->sign_transaction(tx, broadcast);
}

development_committee_api_obj wallet_api::get_development_committee()
{
    return my->_remote_db->get_development_committee();
}

atomicswap_contract_result_api_obj wallet_api::atomicswap_initiate(const std::string& initiator,
                                                                   const std::string& participant,
                                                                   const asset& amount,
                                                                   const std::string& metadata,
                                                                   const uint8_t secret_length,
                                                                   const bool broadcast)
{
    FC_ASSERT(!is_locked());

    std::string secret;

    secret = scorum::wallet::suggest_brain_key().brain_priv_key;
    secret = atomicswap::get_secret_hex(secret, secret_length);

    std::string secret_hash = atomicswap::get_secret_hash(secret);

    atomicswap_initiate_operation op;

    op.type = atomicswap_initiate_operation::by_initiator;
    op.owner = initiator;
    op.recipient = participant;
    op.secret_hash = secret_hash;
    op.amount = amount;
    op.metadata = metadata;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    annotated_signed_transaction ret;
    try
    {
        ret = my->sign_transaction(tx, broadcast);

        return atomicswap_contract_result_api_obj(ret, op, secret);
    }
    catch (fc::exception& e)
    {
        elog("Can't initiate Atomic Swap.");
    }

    return atomicswap_contract_result_api_obj(ret);
}

atomicswap_contract_result_api_obj wallet_api::atomicswap_participate(const std::string& secret_hash,
                                                                      const std::string& participant,
                                                                      const std::string& initiator,
                                                                      const asset& amount,
                                                                      const std::string& metadata,
                                                                      const bool broadcast)
{
    FC_ASSERT(!is_locked());

    atomicswap_initiate_operation op;

    op.type = atomicswap_initiate_operation::by_participant;
    op.owner = participant;
    op.recipient = initiator;
    op.secret_hash = secret_hash;
    op.amount = amount;
    op.metadata = metadata;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    annotated_signed_transaction ret;
    try
    {
        ret = my->sign_transaction(tx, broadcast);

        return atomicswap_contract_result_api_obj(ret, op);
    }
    catch (fc::exception& e)
    {
        elog("Can't participate Atomic Swap.");
    }

    return atomicswap_contract_result_api_obj(ret);
}

annotated_signed_transaction wallet_api::atomicswap_redeem(const std::string& from,
                                                           const std::string& to,
                                                           const std::string& secret,
                                                           const bool broadcast)
{
    FC_ASSERT(!is_locked());

    atomicswap_redeem_operation op;

    op.from = from;
    op.to = to;
    op.secret = secret;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    annotated_signed_transaction ret;
    try
    {
        ret = my->sign_transaction(tx, broadcast);
    }
    catch (fc::exception& e)
    {
        elog("Can't redeem Atomic Swap contract.");
    }

    return ret;
}

atomicswap_contract_info_api_obj
wallet_api::atomicswap_auditcontract(const std::string& from, const std::string& to, const std::string& secret_hash)
{
    atomicswap_contract_info_api_obj ret;
    try
    {
        ret = my->_remote_db->get_atomicswap_contract(from, to, secret_hash);
    }
    catch (fc::exception& e)
    {
        elog("Can't access to Atomic Swap contract.");
    }
    return ret;
}

std::string
wallet_api::atomicswap_extractsecret(const std::string& from, const std::string& to, const std::string& secret_hash)
{
    try
    {
        atomicswap_contract_info_api_obj contract_info = atomicswap_auditcontract(from, to, secret_hash);

        FC_ASSERT(!contract_info.secret.empty(), "Contract is not redeemed.");

        return contract_info.secret;
    }
    catch (fc::exception& e)
    {
        elog("Can't access to Atomic Swap contract secret.");
    }

    return "";
}

annotated_signed_transaction wallet_api::atomicswap_refund(const std::string& participant,
                                                           const std::string& initiator,
                                                           const std::string& secret_hash,
                                                           const bool broadcast)
{
    FC_ASSERT(!is_locked());

    atomicswap_refund_operation op;

    op.participant = participant;
    op.initiator = initiator;
    op.secret_hash = secret_hash;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.validate();

    annotated_signed_transaction ret;
    try
    {
        ret = my->sign_transaction(tx, broadcast);
    }
    catch (fc::exception& e)
    {
        elog("Can't refund Atomic Swap contract.");
    }

    return ret;
}

std::vector<atomicswap_contract_api_obj> wallet_api::get_atomicswap_contracts(const std::string& owner)
{
    std::vector<atomicswap_contract_api_obj> result;

    result = my->_remote_db->get_atomicswap_contracts(owner);

    return result;
}

void wallet_api::exit()
{
    exit_func();
}

chain_capital_api_obj wallet_api::get_chain_capital() const
{
    return my->_chain_api->get_chain_capital();
}

} // namespace wallet
} // namespace scorum
