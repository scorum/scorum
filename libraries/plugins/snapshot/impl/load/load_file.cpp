#include <snapshot/load/load_file.hpp>

#include <fc/io/raw.hpp>
#ifdef SNAPSHOT_DEBUG_OBJECT
#include <fc/io/json.hpp>
#endif

#include <scorum/protocol/block.hpp>
#include <chainbase/db_state.hpp>

#include <snapshot/snapshot_fmt.hpp>
#include <snapshot/data_struct_hash.hpp>
#include <snapshot/find_type_by_id.hpp>

#define DEBUG_SNAPSHOT_LOAD_CONTEXT std::string("load")

namespace scorum {
namespace snapshot {

template <class IterationTag> class load_index_visitor
{
public:
    load_index_visitor(std::ifstream& fstream, db_state& state)
        : _fstream(fstream)
        , _state(state)
    {
    }

    template <class T> void operator()(const T*) const
    {
        using object_type = T;

#ifdef SNAPSHOT_DEBUG_OBJECT
        std::string ctx(DEBUG_SNAPSHOT_LOAD_CONTEXT);
        auto object_name = boost::core::demangle(typeid(object_type).name());
        int debug_id = -1;
        if (object_name.rfind(BOOST_PP_STRINGIZE(SNAPSHOT_DEBUG_OBJECT)) != std::string::npos)
        {
            debug_id = (int)object_type::type_id;
        }
        snapshot_log(ctx, "loading ${name}:${id}", ("name", object_name)("id", (int)object_type::type_id));
#endif // SNAPSHOT_DEBUG_OBJECT

        size_t sz = 0;
        fc::raw::unpack(_fstream, sz);

        const auto& index = _state.template get_index<typename chainbase::get_index_type<object_type>::type>()
                                .indices()
                                .template get<IterationTag>();

        object_type tmp([](object_type&) {}, index.get_allocator());

        if (sz > 0)
        {
            fc::ripemd160 check, etalon;

            fc::raw::unpack(_fstream, check);

            FC_ASSERT(check == get_data_struct_hash(tmp));
        }

        using object_id_type = typename object_type::id_type;
        using objects_copy_type = fc::shared_map<object_id_type, object_type>;
        using object_ref_type = std::reference_wrapper<const object_type>;
        using objects_type = std::vector<object_ref_type>;

        objects_copy_type objs_to_modify(index.get_allocator());

#ifdef SNAPSHOT_DEBUG_OBJECT
        if (debug_id == object_type::type_id)
        {
            snapshot_log(ctx, "debugging ${name}:${id}", ("name", object_name)("id", (int)object_type::type_id));
            snapshot_log(ctx, "loading size=${f}", ("f", sz));
            snapshot_log(ctx, "index size=${sz}", ("sz", index.size()));

            for (auto itr = index.begin(); itr != index.end(); ++itr)
            {
                const object_type& obj = (*itr);

                fc::variant vo;
                fc::to_variant(obj, vo);
                snapshot_log(ctx, "exists ${name}: ${obj}",
                             ("name", object_name)("obj", fc::json::to_pretty_string(vo)));
            }
        }
#endif // SNAPSHOT_DEBUG_OBJECT

        if (index.size() > 0)
        {
            objects_type objs_to_remove;
            objs_to_remove.reserve(index.size());

            for (auto itr = index.begin(); itr != index.end(); ++itr)
            {
                const object_type& obj = (*itr);
                auto id = obj.id;

                if (sz > 0)
                {
                    fc::raw::unpack(_fstream, tmp);
                    auto loaded_id = tmp.id;
                    objs_to_modify.emplace(std::make_pair(loaded_id, tmp));

                    --sz;
                }

                if (!sz || objs_to_modify.find(id) == objs_to_modify.end())
                {
                    objs_to_remove.emplace_back(std::cref(obj));
                }
            }

            for (const object_type& obj : objs_to_remove)
            {
#ifdef SNAPSHOT_DEBUG_OBJECT
                if (debug_id == object_type::type_id)
                {
                    fc::variant vo;
                    fc::to_variant(obj, vo);
                    snapshot_log(ctx, "removed ${name}: ${obj}",
                                 ("name", object_name)("obj", fc::json::to_pretty_string(vo)));
                }
#endif // SNAPSHOT_DEBUG_OBJECT
                _state.template remove(obj);
            }
        }

        for (auto& item : objs_to_modify)
        {
            const object_type* pobj = _state.template find<object_type>(item.first);
            if (pobj != nullptr)
            {
                _state.template modify<object_type>(*pobj, [&](object_type& obj) {
                    obj = std::move(item.second);
#ifdef SNAPSHOT_DEBUG_OBJECT
                    if (debug_id == object_type::type_id)
                    {
                        fc::variant vo;
                        fc::to_variant(obj, vo);
                        snapshot_log(ctx, "updated ${name}: ${obj}",
                                     ("name", object_name)("obj", fc::json::to_pretty_string(vo)));
                    }
#endif // SNAPSHOT_DEBUG_OBJECT
                });
            }
            else
            {
                _state.template create<object_type>([&](object_type& obj) {
                    obj = std::move(item.second);
#ifdef SNAPSHOT_DEBUG_OBJECT
                    if (debug_id == object_type::type_id)
                    {
                        fc::variant vo;
                        fc::to_variant(obj, vo);
                        snapshot_log(ctx, "created ${name}: ${obj}",
                                     ("name", object_name)("obj", fc::json::to_pretty_string(vo)));
                    }
#endif // SNAPSHOT_DEBUG_OBJECT
                });
            }
        }

        objs_to_modify.clear();

        while (sz--)
        {
            _state.template create<object_type>([&](object_type& obj) {
                fc::raw::unpack(_fstream, obj);
#ifdef SNAPSHOT_DEBUG_OBJECT
                if (debug_id == object_type::type_id)
                {
                    fc::variant vo;
                    fc::to_variant(obj, vo);
                    snapshot_log(ctx, "created ${name}: ${obj}",
                                 ("name", object_name)("obj", fc::json::to_pretty_string(vo)));
                }
#endif // SNAPSHOT_DEBUG_OBJECT
            });
        }
    }

    void operator()(const empty_object_type*) const
    {
    }

private:
    std::ifstream& _fstream;
    db_state& _state;
};

load_algorithm::load_algorithm(db_state& state, const fc::path& snapshot_path)
    : _state(state)
    , _snapshot_path(snapshot_path)
{
}

snapshot_header load_algorithm::load_header()
{
    std::ifstream snapshot_stream;
    snapshot_stream.open(_snapshot_path.generic_string(), std::ios::binary);

    snapshot_header header = load_header(snapshot_stream);
    FC_ASSERT(header.version == SCORUM_SNAPSHOT_SERIALIZER_VER);

    snapshot_stream.close();

    return header;
}

snapshot_header load_algorithm::load_header(std::ifstream& fs)
{
    snapshot_header header;
    fc::raw::unpack(fs, header);

    return header;
}

static find_object_type object_types;

void load_algorithm::load()
{
    std::ifstream snapshot_stream;
    snapshot_stream.open(_snapshot_path.generic_string(), std::ios::binary);
    FC_ASSERT(snapshot_stream, "Can't open ${p}", ("p", _snapshot_path.generic_string()));

    uint64_t sz = fc::file_size(_snapshot_path);

    snapshot_header header = load_header(snapshot_stream);

    ilog("Loading snapshot for block ${n} from file ${f}.",
         ("n", header.head_block_number)("f", _snapshot_path.generic_string()));

    using index_ids_type = fc::flat_set<uint16_t>;

    index_ids_type loaded_idxs;

    loaded_idxs.reserve(_state.get_indexes_size());

    _state.for_each_index_key([&](uint16_t index_id) {
        if (object_types.apply(scorum::snapshot::load_index_visitor<by_id>(snapshot_stream, _state), index_id))
        {
            loaded_idxs.insert(index_id);
        }
    });

#ifdef SNAPSHOT_DEBUG_OBJECT
    snapshot_log(DEBUG_SNAPSHOT_LOAD_CONTEXT, "state index size=${sz}, loaded=${lsz}",
                 ("sz", _state.get_indexes_size())("lsz", loaded_idxs.size()));
#endif // SNAPSHOT_DEBUG_OBJECT

    if ((uint64_t)snapshot_stream.tellg() != sz)
    {
        wlog("Not all indexes from snapshot are loaded. Node configuration does not match snapshot.");
    }

    snapshot_stream.close();
}
}
}
