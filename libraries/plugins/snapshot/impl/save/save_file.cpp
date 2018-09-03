#include <snapshot/save/save_file.hpp>

#include <string>
#include <fstream>

#include <fc/io/raw.hpp>
#ifdef SNAPSHOT_DEBUG_OBJECT
#include <fc/io/json.hpp>
#endif

#include <scorum/protocol/block.hpp>
#include <chainbase/db_state.hpp>

#include <snapshot/snapshot_fmt.hpp>
#include <snapshot/data_struct_hash.hpp>
#include <snapshot/find_type_by_id.hpp>

#define DEBUG_SNAPSHOT_SAVE_CONTEXT std::string("save")

namespace scorum {
namespace snapshot {

template <class IterationTag> class save_index_visitor
{
public:
    save_index_visitor(std::ofstream& fstream, db_state& state)
        : _fstream(fstream)
        , _state(state)
    {
    }

    template <class T> void operator()(const T*) const
    {
        using object_type = T;

#ifdef SNAPSHOT_DEBUG_OBJECT
        std::string ctx(DEBUG_SNAPSHOT_SAVE_CONTEXT);
        auto object_name = boost::core::demangle(typeid(object_type).name());
        int debug_id = -1;
        if (object_name.rfind(BOOST_PP_STRINGIZE(SNAPSHOT_DEBUG_OBJECT)) != std::string::npos)
        {
            debug_id = (int)object_type::type_id;
        }
        snapshot_log(ctx, "saving index of ${name}:${id}", ("name", object_name)("id", (int)object_type::type_id));
#endif // SNAPSHOT_DEBUG_OBJECT

        const auto& index = _state.template get_index<typename chainbase::get_index_type<object_type>::type>()
                                .indices()
                                .template get<IterationTag>();
        size_t sz = index.size();

#ifdef SNAPSHOT_DEBUG_OBJECT
        if (debug_id == object_type::type_id)
        {
            snapshot_log(ctx, "debugging ${name}:${id}", ("name", object_name)("id", (int)object_type::type_id));
            snapshot_log(ctx, "index size=${sz}", ("sz", sz));
        }
#endif // SNAPSHOT_DEBUG_OBJECT

        fc::raw::pack(_fstream, sz);
        if (sz > 0)
        {
            auto itr = index.begin();

            fc::raw::pack(_fstream, get_data_struct_hash(*itr));

            for (; itr != index.end(); ++itr)
            {
                const object_type& obj = (*itr);
#ifdef SNAPSHOT_DEBUG_OBJECT
                if (debug_id == object_type::type_id)
                {
                    fc::variant vo;
                    fc::to_variant(obj, vo);
                    snapshot_log(ctx, "saved ${name}: ${obj}",
                                 ("name", object_name)("obj", fc::json::to_pretty_string(vo)));
                }
#endif // SNAPSHOT_DEBUG_OBJECT
                fc::raw::pack(_fstream, obj);
            }
        }
    }

    void operator()(const empty_object_type*) const
    {
    }

private:
    std::ofstream& _fstream;
    db_state& _state;
};

save_algorithm::save_algorithm(db_state& state, const signed_block& block, const fc::path& snapshot_path)
    : _state(state)
    , _block(block)
    , _snapshot_path(snapshot_path)
{
}

static find_object_type object_types;

void save_algorithm::save()
{
    fc::remove_all(_snapshot_path);

    std::ofstream snapshot_stream;
    snapshot_stream.open(_snapshot_path.generic_string(), std::ios::binary);

    snapshot_header header;
    header.version = SCORUM_SNAPSHOT_SERIALIZER_VER;
    header.head_block_number = _block.block_num();
    header.head_block_digest = _block.digest();
    fc::raw::pack(snapshot_stream, header);

    _state.for_each_index_key([&](uint16_t index_id) {
        FC_ASSERT(object_types.apply(save_index_visitor<by_id>(snapshot_stream, _state), index_id), "Can't save index");
    });

    snapshot_stream.close();
}
}
}
