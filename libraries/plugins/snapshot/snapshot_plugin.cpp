#include <scorum/snapshot/snapshot_plugin.hpp>

#include <boost/program_options.hpp>

#include <fc/filesystem.hpp>

#include <scorum/chain/database/database.hpp>

#include <snapshot/save/saving_task.hpp>
#include <snapshot/load/loading_task.hpp>

namespace scorum {
namespace snapshot {

namespace bpo = boost::program_options;

namespace detail {
using namespace scorum::chain;

class snapshot_plugin_impl
{
public:
    snapshot_plugin_impl(snapshot_plugin& plugin)
        : _self(plugin)
        , _saver(_self.database())
        , _loader(_self.database())
    {
    }

    void plugin_initialize(const bpo::variables_map& options)
    {
        if (options.count("snapshot-save-dir"))
        {
            fc::path snapshot_dir = fc::path(options.at("snapshot-save-dir").as<boost::filesystem::path>());
            snapshot_dir = boost::filesystem::absolute(snapshot_dir);
            snapshot_dir = ensure_snapshot_dir(snapshot_dir);
            _saver.set_snapshot_dir(snapshot_dir);
            _loader.set_snapshot_dir(snapshot_dir);
        }

        if (options.count("load-snapshot-file"))
        {
            fc::path snapshot_file;
            snapshot_file = fc::path(options.at("load-snapshot-file").as<boost::filesystem::path>());
            snapshot_file = boost::filesystem::absolute(snapshot_file);
            _loader.set_snapshot_file(snapshot_file);
        }

        if (options.count("save-snapshot-number"))
        {
            auto number = options.at("save-snapshot-number").as<int32_t>();
            if (number >= 0)
            {
                _saver.schedule_snapshot((uint32_t)number);
            }
        }

        chain::database& db = _self.database();

        db.applied_block.connect([&](const signed_block& b) { on_block(b); });
        db.runtime_config.connect([&](const database::runtime_config_mode& m) { on_runtime_config(m); });
    }

    uint32_t get_snapshot_number() const
    {
        return _loader.get_snapshot_number();
    }

private:
    void on_runtime_config(const database::runtime_config_mode& m)
    {
        if (m == database::runtime_config_mode::sig1)
        {
            _saver.schedule_snapshot();
        }
    }

    void on_block(const signed_block& block)
    {
        _loader.apply(block);
        _saver.apply(block);
    }

    fc::path ensure_snapshot_dir(const fc::path& dir)
    {
        fc::path result;
        try
        {
            if (dir == fc::path())
            {
                result = _self.database().data_dir();
            }
            else
            {
                if (!fc::exists(dir))
                {
                    fc::create_directories(dir);
                }
                result = dir;
            }
        }
        FC_CAPTURE_AND_RETHROW((dir))
        return result;
    }

    snapshot_plugin& _self;

    saving_task _saver;
    loading_task _loader;
};
}

snapshot_plugin::snapshot_plugin(application* app)
    : plugin(app)
    , _impl(new detail::snapshot_plugin_impl(*this))
{
}

snapshot_plugin::~snapshot_plugin()
{
}

void snapshot_plugin::plugin_set_program_options(bpo::options_description& command_line_options,
                                                 bpo::options_description& config_file_options)
{
    // clang-format off
    command_line_options.add_options()
    ("snapshot-save-dir", bpo::value<boost::filesystem::path>(), "Directory for snapshots saving. Default is data_dir.")
    ("load-snapshot-file", bpo::value<boost::filesystem::path>(), "File with snapshot to load.")
    ("save-snapshot-number", bpo::value< int32_t >()->default_value(-1), "Schedule snapshot for this block number. "
                                                                         "By default (-1) there is no snapshot is scheduled."
                                                                         "Use 0 to snapshot the first applied block (not only genesis)."
                                                                         "This is the same if the node receive SIGUSR1.");
    // clang-format on

    config_file_options.add(command_line_options);
}

std::string snapshot_plugin::plugin_name() const
{
    return SNAPSHOT_PLUGIN_NAME;
}

void snapshot_plugin::plugin_initialize(const bpo::variables_map& options)
{
    try
    {
        _impl->plugin_initialize(options);
    }
    FC_LOG_AND_RETHROW()

    print_greeting();
}

uint32_t snapshot_plugin::get_snapshot_number() const
{
    return _impl->get_snapshot_number();
}
}
} // scorum::witness

SCORUM_DEFINE_PLUGIN(snapshot, scorum::snapshot::snapshot_plugin)
