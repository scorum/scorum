//#ifdef IS_TEST_NET
//#include <boost/test/unit_test.hpp>

//#include <scorum/protocol/exceptions.hpp>

//#include <scorum/protocol/types.hpp>
//#include <scorum/chain/dbs_committee_proposal.hpp>
//#include <scorum/chain/proposal_vote_evaluator.hpp>
//#include <scorum/chain/proposal_vote_object.hpp>

// namespace scorum {
// namespace chain {

// class account_service_mock
//{
// public:
//    check_account_existence()
//    {
//        check_account_existence = true;
//    }

//    bool b_check_account_existence = false;
//};

// class proposal_service_mock
//{
// public:
//    vote_for()
//    {
//        b_vote_for = true;
//    }

//    remove()
//    {
//        b_remove = true;
//    }

//    bool b_vote_for = false;
//    bool b_remove = false;
//};

// class committee_service_mock
//{
// public:
//    void add_member(const account_name_type&)
//    {
//        b_add_member = true;
//    }

//    void exclude_member(const account_name_type&)
//    {
//        b_exclude_member = true;
//    }

//    bool b_add_member = false;
//    bool b_exclude_member = false;
//};

// typedef proposal_vote_evaluator_t<account_service_mock, proposal_service_mock, committee_service_mock>
// evaluator_mocked;

// class proposal_evaluator_fixture : public evaluator_mocked
//{
// public:
//    proposal_evaluator_fixture()
//        : evaluator_mocked()
//    {
//    }
//};
//}
//}

// BOOST_FIXTURE_TEST_SUITE(proposal_vote_evaluator_tests, proposal_evaluator_fixture)

// BOOST_AUTO_TEST_CASE(add_new_member)
//{

//}

// BOOST_AUTO_TEST_CASE(dropout_member)
//{

//}

// BOOST_AUTO_TEST_CASE(proposal_removed_after_adding_member)
//{

//}

// BOOST_AUTO_TEST_CASE(proposal_removed_after_droping_out_member)
//{

//}

// BOOST_AUTO_TEST_CASE(fail_on_voting_account_unexistent)
//{

//}

// BOOST_AUTO_TEST_CASE(fail_on_proposal_expired)
//{

//}

// BOOST_AUTO_TEST_SUITE_END()

//#endif
