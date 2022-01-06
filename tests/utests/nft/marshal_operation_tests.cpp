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
    op.power = 100;

    operation o(op);

    BOOST_CHECK_EQUAL(to_hex(o), "2b086f70657261746f72aa3b2bdc176e5e8a9b487dba3aa1004406726f636b6574027b7d6400000000000000");
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

    BOOST_CHECK_EQUAL(to_hex(o), "2d086f70657261746f72aa3b2bdc176e5e8a9b487dba3aa1004440303338623266626634653466303636663330393939316239633330636238663838373835336535346337366463373035663565636537333665616436633835364030353264386265633664353566386334383965383337633139666133373265303065333433336562666530303638616636353865333662306264316562373232");
}

SCORUM_TEST_CASE(game_round_result_marshall)
{
    game_round_result_operation op;
    op.owner = "operator";
    op.uuid = gen_uuid("first");
    op.proof = "638f675cd4313ae84aede4940b7691acd904dec141e444187dcec59f2a25a7a4ef5aa2fe3f88cf235c0d63aa6935bef69d5b70caca0d9b4028f75121d030f80a5a4bcf97b36a868ea9a4c2aaa9013200";
    op.vrf = "6a196a14e4f9fce66112b1b7ac98f2bcd73352b918d298e0b9f894519a65202dba03ddaa5183190bf2b5cd551f9ef14c8d8b02cf15d0188bbc9bcc6a80d7f91c";
    op.result = 100;

    operation o(op);

    BOOST_CHECK_EQUAL(to_hex(o), "2e086f70657261746f72aa3b2bdc176e5e8a9b487dba3aa10044a00136333866363735636434333133616538346165646534393430623736393161636439303464656331343165343434313837646365633539663261323561376134656635616132666533663838636632333563306436336161363933356265663639643562373063616361306439623430323866373531323164303330663830613561346263663937623336613836386561396134633261616139303133323030800136613139366131346534663966636536363131326231623761633938663262636437333335326239313864323938653062396638393435313961363532303264626130336464616135313833313930626632623563643535316639656631346338643862303263663135643031383862626339626363366138306437663931636400000000000000");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace