#include <boost/test/unit_test.hpp>

#include <scorum/chain/evaluators/create_game_round_evaluator.hpp>

#include <hippomocks.h>
#include <defines.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "detail.hpp"

using ::detail::to_hex;

namespace {

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

BOOST_AUTO_TEST_SUITE(nft_operaion_marshall_test)

SCORUM_TEST_CASE(create_nft_operation_marshall)
{
    create_nft_operation op;
    op.owner = "operator";
    op.uuid = gen_uuid("first");
    op.name = "rocket";
    op.json_metadata = "{}";
    op.initial_power = 100;

    operation o(op);

    BOOST_CHECK_EQUAL(to_hex(o), "2b086f70657261746f72aa3b2bdc176e5e8a9b487dba3aa1004406726f636b6574027b7d64000000");
}

SCORUM_TEST_CASE(update_nft_meta_operation_marshall)
{
    update_nft_meta_operation op;
    op.moderator = "operator";
    op.uuid = gen_uuid("first");
    op.json_metadata = "{}";

    operation o(op);

    BOOST_CHECK_EQUAL(to_hex(o), "2c086f70657261746f72aa3b2bdc176e5e8a9b487dba3aa10044027b7d");
}

SCORUM_TEST_CASE(create_game_round_operation_marshall)
{
    create_game_round_operation op;
    op.owner = "operator";
    op.uuid = gen_uuid("first");
    op.seed = "052d8bec6d55f8c489e837c19fa372e00e3433ebfe0068af658e36b0bd1eb722";
    op.verification_key = "038b2fbf4e4f066f309991b9c30cb8f887853e54c76dc705f5ece736ead6c856";

    operation o(op);

    BOOST_CHECK_EQUAL(
        to_hex(o),
        "2d086f70657261746f72aa3b2bdc176e5e8a9b487dba3aa100444030333862326662663465346630363666333039393931623963333063"
        "62386638383738353365353463373664633730356635656365373336656164366338353640303532643862656336643535663863343839"
        "65383337633139666133373265303065333433336562666530303638616636353865333662306264316562373232");
}

SCORUM_TEST_CASE(game_round_result_marshall)
{
    update_game_round_result_operation op;
    op.owner = "operator";
    op.uuid = gen_uuid("first");
    op.proof = "638f675cd4313ae84aede4940b7691acd904dec141e444187dcec59f2a25a7a4ef5aa2fe3f88cf235c0d63aa6935bef69d5b70c"
               "aca0d9b4028f75121d030f80a5a4bcf97b36a868ea9a4c2aaa9013200";
    op.vrf = "6a196a14e4f9fce66112b1b7ac98f2bcd73352b918d298e0b9f894519a65202dba03ddaa5183190bf2b5cd551f9ef14c8d8b02cf1"
             "5d0188bbc9bcc6a80d7f91c";
    op.result = 100;

    operation o(op);

    BOOST_CHECK_EQUAL(
        to_hex(o),
        "2e086f70657261746f72aa3b2bdc176e5e8a9b487dba3aa10044a001363338663637356364343331336165383461656465343934306237"
        "36393161636439303464656331343165343434313837646365633539663261323561376134656635616132666533663838636632333563"
        "30643633616136393335626566363964356237306361636130643962343032386637353132316430333066383061356134626366393762"
        "33366138363865613961346332616161393031333230308001366131393661313465346639666365363631313262316237616339386632"
        "62636437333335326239313864323938653062396638393435313961363532303264626130336464616135313833313930626632623563"
        "6435353166396566313463386438623032636631356430313838626263396263633661383064376639316364000000");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace