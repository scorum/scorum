#pragma once

#include <limits>
#include <chainbase/generic_index.hpp>
#include <scorum/protocol/block.hpp>
#include <scorum/chain/operation_notification.hpp>

#define LIFE_TIME_PERIOD std::numeric_limits<uint32_t>::max()

namespace scorum {
namespace common_statistics {

using namespace scorum::protocol;
using namespace scorum::chain;

struct by_bucket;

template <typename Bucket, typename Plugin> class common_statistics_plugin_impl
{
    typedef typename chainbase::get_index_type<Bucket>::type bucket_index;

public:
    typedef common_statistics_plugin_impl base_plugin_impl;

    Plugin& _self;
    flat_set<uint32_t> _tracked_buckets = { 60, 3600, 21600, 86400, 604800, 2592000, LIFE_TIME_PERIOD };
    flat_set<typename Bucket::id_type> _current_buckets;
    uint32_t _maximum_history_per_bucket_size = 100;

public:
    common_statistics_plugin_impl(Plugin& plugin)
        : _self(plugin)
    {
    }
    virtual ~common_statistics_plugin_impl()
    {
    }

    virtual void process_bucket_creation(const Bucket& bucket)
    {
    }
    virtual void process_block(const Bucket& bucket, const signed_block& b)
    {
    }
    virtual void process_pre_operation(const Bucket& bucket, const operation_notification& o)
    {
    }
    virtual void process_post_operation(const Bucket& bucket, const operation_notification& o)
    {
    }
    virtual void process_pre_proposal_operation(const Bucket& bucket, const proposal_operation_notification& o)
    {
    }

    void initialize()
    {
        auto& db = _self.database();

        db.applied_block.connect([&](const signed_block& b) { this->on_block(b); });
        db.pre_apply_operation.connect([&](const operation_notification& o) { this->pre_operation(o); });
        db.post_apply_operation.connect([&](const operation_notification& o) { this->post_operation(o); });
        db.pre_apply_proposal_operation.connect([&](const proposal_operation_notification& o) { this->pre_proposal_operation(o); });

        db.template add_plugin_index<bucket_index>();
    }

    void pre_operation(const operation_notification& o)
    {
        auto& db = _self.database();

        for (auto bucket_id : _current_buckets)
        {
            const auto& bucket = db.get(bucket_id);

            process_pre_operation(bucket, o);
        }
    }

    void post_operation(const operation_notification& o)
    {
        try
        {
            auto& db = _self.database();

            for (auto bucket_id : _current_buckets)
            {
                const auto& bucket = db.get(bucket_id);

                process_post_operation(bucket, o);
            }
        }
        FC_CAPTURE_AND_RETHROW()
    }

    void pre_proposal_operation(const proposal_operation_notification& o)
    {
        auto& db = _self.database();

        for (auto bucket_id : _current_buckets)
        {
            const auto& bucket = db.get(bucket_id);

            process_pre_proposal_operation(bucket, o);
        }
    }

    void on_block(const signed_block& block)
    {
        auto& db = _self.database();

        _current_buckets.clear();

        const auto& bucket_idx = db.template get_index<bucket_index>().indices().get<common_statistics::by_bucket>();

        for (const auto& bucket : _tracked_buckets)
        {
            auto open = fc::time_point_sec((db.head_block_time().sec_since_epoch() / bucket) * bucket);

            auto itr = bucket_idx.find(boost::make_tuple(bucket, open));
            if (itr != bucket_idx.end())
            {
                _current_buckets.insert(itr->id);
            }
            else
            {
                const auto& new_bucket_obj = db.template create<Bucket>([&](Bucket& bo) {
                    bo.open = open;
                    bo.seconds = bucket;
                });

                process_bucket_creation(new_bucket_obj);

                _current_buckets.insert(new_bucket_obj.id);

                // adjust history
                if (_maximum_history_per_bucket_size > 0)
                {
                    try
                    {
                        auto cutoff = fc::time_point_sec();
                        if (safe<uint64_t>(bucket) * safe<uint64_t>(_maximum_history_per_bucket_size)
                            < safe<uint64_t>(LIFE_TIME_PERIOD))
                        {
                            cutoff = fc::time_point_sec(
                                (safe<uint32_t>(db.head_block_time().sec_since_epoch())
                                 - safe<uint32_t>(bucket) * safe<uint32_t>(_maximum_history_per_bucket_size))
                                    .value);
                        }

                        itr = bucket_idx.lower_bound(boost::make_tuple(bucket, fc::time_point_sec()));

                        while (itr->seconds == bucket && itr->open < cutoff)
                        {
                            auto old_itr = itr;
                            ++itr;
                            db.remove(*old_itr);
                        }
                    }
                    catch (fc::overflow_exception& e)
                    {
                    }
                    catch (fc::underflow_exception& e)
                    {
                    }
                }
            }

            process_block(*itr, block);
        }
    }
};

} // namespace common_statistics
} // namespace scorum
