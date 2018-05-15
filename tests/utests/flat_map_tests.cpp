#include <boost/test/unit_test.hpp>

#include <boost/container/flat_map.hpp>

#include "defines.hpp"

#include <string>
#include <map>
#include <vector>

#include <random>

BOOST_AUTO_TEST_SUITE(flat_map_tests)

BOOST_AUTO_TEST_CASE(nth_check)
{
    using std_ids_map_type = std::map<int64_t, std::string>;
    using insert_order_type = std::vector<int64_t>;
    using flat_ids_map_type = boost::container::flat_map<int64_t, std::string>;

    std_ids_map_type std_data;

    std_data[100] = "alice";
    std_data[200] = "bob";
    std_data[302] = "sam";
    std_data[455] = "john";
    std_data[1455] = "john2";
    std_data[2405] = "andrew";
    std_data[1] = "scorumwitness1";
    std_data[2] = "scorumwitness2";
    std_data[3] = "scorumwitness3";
    std_data[4] = "scorumwitness4";
    std_data[5] = "scorumwitness5";
    std_data[6] = "scorumwitness6";
    std_data[7] = "scorumwitness7";
    std_data[8] = "scorumwitness8";
    std_data[9] = "scorumwitness9";
    std_data[10] = "scorumwitness10";
    std_data[11] = "scorumwitness11";
    std_data[12] = "scorumwitness12";

    const size_t data_sz = std_data.size();

    insert_order_type insert_order;
    insert_order.reserve(data_sz);

    for (const auto& p : std_data)
    {
        insert_order.push_back(p.first);
    }

    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, data_sz - 1);

    for (int ck = 0; ck < 100; ++ck)
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

BOOST_AUTO_TEST_SUITE_END()
