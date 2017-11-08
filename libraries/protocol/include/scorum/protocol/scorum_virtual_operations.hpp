#pragma once
#include <scorum/protocol/base.hpp>
#include <scorum/protocol/block_header.hpp>
#include <scorum/protocol/asset.hpp>

#include <fc/utf8.hpp>

namespace scorum { namespace protocol {

   struct author_reward_operation : public virtual_operation {
      author_reward_operation(){}
      author_reward_operation( const account_name_type& a, const string& p, const asset& scr, const asset& v )
         :author(a), permlink(p), scorum_payout(scr), vesting_payout(v){}

      account_name_type author;
      string            permlink;
      asset             scorum_payout;
      asset             vesting_payout;
   };


   struct curation_reward_operation : public virtual_operation
   {
      curation_reward_operation(){}
      curation_reward_operation( const string& c, const asset& r, const string& a, const string& p )
         :curator(c), reward(r), comment_author(a), comment_permlink(p) {}

      account_name_type curator;
      asset             reward;
      account_name_type comment_author;
      string            comment_permlink;
   };


   struct comment_reward_operation : public virtual_operation
   {
      comment_reward_operation(){}
      comment_reward_operation( const account_name_type& a, const string& pl, const asset& p )
         :author(a), permlink(pl), payout(p){}

      account_name_type author;
      string            permlink;
      asset             payout;
   };

   struct fill_vesting_withdraw_operation : public virtual_operation
   {
      fill_vesting_withdraw_operation(){}
      fill_vesting_withdraw_operation( const string& f, const string& t, const asset& w, const asset& d )
         :from_account(f), to_account(t), withdrawn(w), deposited(d) {}

      account_name_type from_account;
      account_name_type to_account;
      asset             withdrawn;
      asset             deposited;
   };


   struct shutdown_witness_operation : public virtual_operation
   {
      shutdown_witness_operation(){}
      shutdown_witness_operation( const string& o ):owner(o) {}

      account_name_type owner;
   };

   struct hardfork_operation : public virtual_operation
   {
      hardfork_operation() {}
      hardfork_operation( uint32_t hf_id ) : hardfork_id( hf_id ) {}

      uint32_t         hardfork_id = 0;
   };

   struct comment_payout_update_operation : public virtual_operation
   {
      comment_payout_update_operation() {}
      comment_payout_update_operation( const account_name_type& a, const string& p ) : author( a ), permlink( p ) {}

      account_name_type author;
      string            permlink;
   };

   struct return_vesting_delegation_operation : public virtual_operation
   {
      return_vesting_delegation_operation() {}
      return_vesting_delegation_operation( const account_name_type& a, const asset& v ) : account( a ), vesting_shares( v ) {}

      account_name_type account;
      asset             vesting_shares;
   };

   struct comment_benefactor_reward_operation : public virtual_operation
   {
      comment_benefactor_reward_operation() {}
      comment_benefactor_reward_operation( const account_name_type& b, const account_name_type& a, const string& p, const asset& r )
         : benefactor( b ), author( a ), permlink( p ), reward( r ) {}

      account_name_type benefactor;
      account_name_type author;
      string            permlink;
      asset             reward;
   };

   struct producer_reward_operation : public virtual_operation
   {
      producer_reward_operation(){}
      producer_reward_operation( const string& p, const asset& v ) : producer( p ), vesting_shares( v ) {}

      account_name_type producer;
      asset             vesting_shares;

   };

} } //scorum::protocol

FC_REFLECT( scorum::protocol::author_reward_operation, (author)(permlink)(scorum_payout)(vesting_payout) )
FC_REFLECT( scorum::protocol::curation_reward_operation, (curator)(reward)(comment_author)(comment_permlink) )
FC_REFLECT( scorum::protocol::comment_reward_operation, (author)(permlink)(payout) )
FC_REFLECT( scorum::protocol::fill_vesting_withdraw_operation, (from_account)(to_account)(withdrawn)(deposited) )
FC_REFLECT( scorum::protocol::shutdown_witness_operation, (owner) )
FC_REFLECT( scorum::protocol::hardfork_operation, (hardfork_id) )
FC_REFLECT( scorum::protocol::comment_payout_update_operation, (author)(permlink) )
FC_REFLECT( scorum::protocol::return_vesting_delegation_operation, (account)(vesting_shares) )
FC_REFLECT( scorum::protocol::comment_benefactor_reward_operation, (benefactor)(author)(permlink)(reward) )
FC_REFLECT( scorum::protocol::producer_reward_operation, (producer)(vesting_shares) )
