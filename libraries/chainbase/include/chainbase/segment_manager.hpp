#pragma once

#include <boost/filesystem/path.hpp>

#include <chainbase/generic_index.hpp>

namespace chainbase {

class segment_manager
{
protected:
    bool _read_only = false;

    std::unique_ptr<boost::interprocess::managed_mapped_file> _segment;

protected:
    size_t get_free_memory() const;

    void create_segment_file(const boost::filesystem::path& file, bool read_only, uint64_t shared_file_size);

    void flush_segment_file();

    void close_segment_file();

    template <typename MultiIndexType> generic_index<MultiIndexType>* allocate_index()
    {
        typedef generic_index<MultiIndexType> index_type;

        std::string type_name = boost::core::demangle(typeid(typename index_type::value_type).name());

        index_type* idx_ptr = nullptr;
        if (!_read_only)
        {
            idx_ptr = _segment->find_or_construct<index_type>(type_name.c_str())(_segment->get_segment_manager());
        }
        else
        {
            idx_ptr = _segment->find<index_type>(type_name.c_str()).first;
        }

        // clang-format off
        if (!idx_ptr)
            BOOST_THROW_EXCEPTION(std::runtime_error("unable to find index for " + type_name + " in read " + (_read_only ? "only" : "/ write") + " database"));
        // clang-format on

        return idx_ptr;
    }
};
}
