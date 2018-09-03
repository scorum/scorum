#include <boost/test/unit_test.hpp>

#include "fc/reflect/reflect.hpp"
#include <fc/io/json.hpp>
#include <fc/crypto/hex.hpp>

#include <fc/crypto/base58.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/raw.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <fstream>

#include <fc/crypto/ripemd160.hpp>
#include <fc/container/flat.hpp>

#include <graphene/utilities/tempdir.hpp>

namespace pack_unpack_reflected_obj_tests {

struct test_object_type
{
    test_object_type()
    {
    }
    test_object_type(const std::string& v0, int v1, const std::string& v2)
        : val0(v0)
        , val1(v1)
        , val2(v2)
    {
    }
    std::string val0;
    int val1;
    std::string val2;
};

using test_objects_type = std::vector<test_object_type>;

BOOST_AUTO_TEST_SUITE(pack_unpack_reflected_obj_tests)

BOOST_AUTO_TEST_CASE(binary_reflect)
{
    test_object_type obj_in{ "Object ", 1, "T1" };

    auto buff = fc::raw::pack(obj_in);

    // std::cerr << fc::to_hex(buff) << std::endl;

    test_object_type obj_out;

    fc::raw::unpack<test_object_type>(buff, obj_out);

    BOOST_CHECK_EQUAL(obj_in.val0, obj_out.val0);
    BOOST_CHECK_EQUAL(obj_in.val1, obj_out.val1);
    BOOST_CHECK_EQUAL(obj_in.val2, obj_out.val2);
}

BOOST_AUTO_TEST_CASE(vector_binary_reflect)
{
    static const size_t common_buff_sz = 1024u;
    char common_buff[common_buff_sz];
    memset(common_buff, 0, common_buff_sz);

    test_objects_type v_obj_in;

    v_obj_in.push_back({ "Object ", 1, "T1" });
    v_obj_in.push_back({ "ObjectObject ", 2, "T2" });
    v_obj_in.push_back({ "ObjectObjectObject ", 3, "T3" });

    {
        fc::datastream<char*> ds_in(common_buff, common_buff_sz);

        fc::raw::pack(ds_in, v_obj_in.size());
        for (const auto& val : v_obj_in)
        {
            fc::raw::pack(ds_in, val);
        }
    }

    // std::cerr << fc::to_hex(common_buff, common_buff_sz) << std::endl;

    test_objects_type v_obj_out;

    v_obj_out.reserve(v_obj_in.size());

    {
        fc::datastream<char*> ds_out(common_buff, common_buff_sz);

        size_t cn = 0;
        fc::raw::unpack(ds_out, cn);
        for (size_t ci = 0; ci < (size_t)cn; ++ci)
        {
            test_object_type obj_out;
            fc::raw::unpack(ds_out, obj_out);
            v_obj_out.push_back(obj_out);
        }
    }

    BOOST_REQUIRE_EQUAL(v_obj_in.size(), v_obj_out.size());
    for (size_t ci = 0; ci < v_obj_out.size(); ++ci)
    {
        BOOST_CHECK_EQUAL(v_obj_in[ci].val0, v_obj_out[ci].val0);
        BOOST_CHECK_EQUAL(v_obj_in[ci].val1, v_obj_out[ci].val1);
        BOOST_CHECK_EQUAL(v_obj_in[ci].val2, v_obj_out[ci].val2);
    }
}

BOOST_AUTO_TEST_CASE(vector_binary_save_and_load)
{
    test_objects_type v_obj_in;

    v_obj_in.push_back({ "Object ", 1, "T1" });
    v_obj_in.push_back({ "ObjectObject ", 2, "T2" });
    v_obj_in.push_back({ "ObjectObjectObject ", 3, "T3" });

    static fc::path snapshot_dir_path = fc::temp_directory(graphene::utilities::temp_directory_path()).path();

    fc::create_directories(snapshot_dir_path);

    fc::path snapshot_file_path = snapshot_dir_path / "test.bin";

    {
        std::ofstream snapshot_stream;
        snapshot_stream.open(snapshot_file_path.generic_string(), std::ios::binary);

        fc::ripemd160::encoder check_enc;
        fc::raw::pack(check_enc, __FUNCTION__);
        auto check = check_enc.result();

        fc::raw::pack(snapshot_stream, check);

        {
            fc::raw::pack(snapshot_stream, v_obj_in.size());
            for (const auto& val : v_obj_in)
            {
                fc::raw::pack(snapshot_stream, val);
            }
        }

        snapshot_stream.close();
    }

    test_objects_type v_obj_out;

    v_obj_out.reserve(v_obj_in.size());

    {
        std::ifstream snapshot_stream;
        snapshot_stream.open(snapshot_file_path.generic_string(), std::ios::binary);

        {
            fc::ripemd160 check;

            fc::raw::unpack(snapshot_stream, check);

            fc::ripemd160::encoder check_enc;
            fc::raw::pack(check_enc, __FUNCTION__);

            BOOST_REQUIRE(check_enc.result() == check);

            size_t cn = 0;
            fc::raw::unpack(snapshot_stream, cn);
            for (size_t ci = 0; ci < (size_t)cn; ++ci)
            {
                test_object_type obj_out;
                fc::raw::unpack(snapshot_stream, obj_out);
                v_obj_out.push_back(obj_out);
            }
        }

        snapshot_stream.close();
    }

    BOOST_REQUIRE_EQUAL(v_obj_in.size(), v_obj_out.size());
    for (size_t ci = 0; ci < v_obj_out.size(); ++ci)
    {
        BOOST_CHECK_EQUAL(v_obj_in[ci].val0, v_obj_out[ci].val0);
        BOOST_CHECK_EQUAL(v_obj_in[ci].val1, v_obj_out[ci].val1);
        BOOST_CHECK_EQUAL(v_obj_in[ci].val2, v_obj_out[ci].val2);
    }

    fc::remove_all(snapshot_dir_path);
}

BOOST_AUTO_TEST_SUITE_END()
}

FC_REFLECT(pack_unpack_reflected_obj_tests::test_object_type, (val0)(val1)(val2))
