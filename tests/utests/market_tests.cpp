#include <boost/test/unit_test.hpp>

#include <fc/reflect/reflect.hpp>

#include <scorum/protocol/betting/market.hpp>

#include <boost/fusion/container.hpp>

#include <scorum/utils/static_variant_serialization.hpp>

namespace market_tests {

using namespace scorum::protocol;
using namespace scorum::protocol::betting;

template <typename... Wincases> struct base_market_type
{
    base_market_type() = default;

    template <typename Wincase> Wincase create_wincase()
    {
        return boost::fusion::at_key<Wincase>(_wincases);
    }

private:
    boost::fusion::set<Wincases...> _wincases = { Wincases{}... };
};

template <typename... Wincases> struct base_market_with_threshold_type
{
    int16_t threshold = 0;

    base_market_with_threshold_type() = default;

    base_market_with_threshold_type(const int16_t threshold_)
        : threshold(threshold_)
    {
    }

    template <typename Wincase> Wincase create_wincase()
    {
        return boost::fusion::at_key<Wincase>(_wincases);
    }

private:
    boost::fusion::set<Wincases...> _wincases = { Wincases{ threshold }... };
};

template <typename... Wincases> struct base_market_with_score_type
{
    uint16_t home = 0;
    uint16_t away = 0;

    base_market_with_score_type() = default;

    base_market_with_score_type(const int16_t home_, const int16_t away_)
        : home(home_)
        , away(away_)
    {
    }

    template <typename Wincase> Wincase create_wincase()
    {
        return boost::fusion::at_key<Wincase>(_wincases);
    }

private:
    boost::fusion::set<Wincases...> _wincases = { Wincases{ home, away }... };
};

using result_market
    = base_market_type<result_home, result_draw_away, result_draw, result_home_away, result_away, result_home_draw>;
using handicap_market = base_market_with_threshold_type<handicap_home_over, handicap_home_under>;
using correct_score_market = base_market_with_score_type<correct_score_yes, correct_score_no>;

using market_new_type = fc::static_variant<result_market, handicap_market, correct_score_market>;

BOOST_AUTO_TEST_SUITE(market_tests)

BOOST_AUTO_TEST_CASE(market_base_check)
{
    market_new_type market1 = result_market{};
    market_new_type market2 = handicap_market{ 1000 };
    market_new_type market3 = correct_score_market{ 1, 3 };

    wdump((market1));
    wdump((market2));
    wdump((market3));

    // TODO
}

BOOST_AUTO_TEST_SUITE_END()
}

FC_REFLECT_EMPTY(market_tests::result_market)
FC_REFLECT(market_tests::handicap_market, (threshold))
FC_REFLECT(market_tests::correct_score_market, (home)(away))

namespace fc {
using namespace market_tests;

void to_variant(const market_new_type& market, fc::variant& var)
{
    scorum::utils::to_variant(market, var);
}
void from_variant(const fc::variant& var, market_new_type& market)
{
    scorum::utils::from_variant(var, market);
}
}
