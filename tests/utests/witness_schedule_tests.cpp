#include <boost/test/unit_test.hpp>

#include <boost/container/flat_map.hpp>

#include <fc/time.hpp>

#include "defines.hpp"

#include <string>
#include <map>
#include <vector>

#include <random>
#include <iostream>

struct witness_schedule_fixture
{
    witness_schedule_fixture()
    {
        std_data[0] = "scorumwitness1";
        std_data[1] = "scorumwitness2";
        std_data[2] = "scorumwitness3";
        std_data[3] = "scorumwitness4";
        std_data[4] = "scorumwitness5";
        std_data[23] = "alexshkor";
        std_data[24] = "vlad";
        std_data[25] = "kotik";
        std_data[26] = "iromanovsky";
        std_data[27] = "bender";
        std_data[28] = "andrewww";
        std_data[29] = "mercury";
        std_data[30] = "batman";
        std_data[31] = "best-witness";
        std_data[32] = "blckchnd";
        std_data[33] = "abolinbsu";
        std_data[34] = "andrew";
        std_data[35] = "roelandp";
        std_data[36] = "educatedwarrior";
        std_data[37] = "atrigubov";
        std_data[38] = "ruslan";

        data_sz = std_data.size();

        insert_order.reserve(data_sz);

        for (const auto& p : std_data)
        {
            insert_order.push_back(p.first);
        }

        head_block_time = fc::time_point_sec::from_iso_string("2018-03-24T18:14:27");
    }

    using std_ids_map_type = std::map<int64_t, std::string>;
    using insert_order_type = std::vector<int64_t>;

    std_ids_map_type std_data;
    size_t data_sz = 0u;
    insert_order_type insert_order;

    fc::time_point_sec head_block_time;
};

BOOST_AUTO_TEST_SUITE(witness_schedule_tests)

BOOST_FIXTURE_TEST_CASE(flat_map_nth_check, witness_schedule_fixture)
{
    const int test_loop_size = 10;

    using flat_ids_map_type = boost::container::flat_map<int64_t, std::string>;

    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, data_sz - 1);

    for (int ck = 0; ck < test_loop_size; ++ck)
    {
        insert_order_type insert_order_tmp(insert_order);

        for (int ci = data_sz - 1; ci >= 0; --ci)
        {
            int cj = (distribution(generator) % data_sz);
            auto prev_id = insert_order_tmp[cj];
            insert_order_tmp[cj] = insert_order_tmp[ci];
            insert_order_tmp[ci] = prev_id;
        }

        flat_ids_map_type flat_data;
        flat_data.reserve(data_sz);

        for (size_t ci = 0; ci < data_sz; ++ci)
        {
            auto inst_p = std_data.find(insert_order_tmp[ci]);
            flat_data.insert(std::make_pair(inst_p->first, inst_p->second));
        }

        for (size_t ci = 0; ci < data_sz; ++ci)
        {
            BOOST_CHECK_EQUAL(flat_data.nth(ci)->second, std_data[insert_order[ci]]);
        }
    }
}

// BOOST_FIXTURE_TEST_CASE(psevdo_rnd_check, witness_schedule_fixture)
//{
//    insert_order_type insert_order_tmp(insert_order);

//    auto now_hi = uint64_t(head_block_time.sec_since_epoch()) << 32;
//    for (uint32_t i = 0; i < data_sz; ++i)
//    {
//        uint64_t k = now_hi + uint64_t(i) * 2685821657736338717ULL;
//        k ^= (k >> 12);
//        k ^= (k << 25);
//        k ^= (k >> 27);
//        k *= 2685821657736338717ULL;

//        uint32_t jmax = data_sz - i;
//        uint32_t j = i + k % jmax;
//        std::swap(insert_order_tmp[i], insert_order_tmp[j]);
//    }

//    std::cerr << head_block_time.to_iso_string() << ": ";
//    for (size_t ci = 0; ci < insert_order_tmp.size(); ++ci)
//    {
//        std::cerr << "(" << ci + 1 << ", [" << insert_order_tmp[ci] << ", " << std_data[insert_order_tmp[ci]] << "])";
//    }

//    std::cerr << std::endl;
//}

BOOST_AUTO_TEST_SUITE_END()
