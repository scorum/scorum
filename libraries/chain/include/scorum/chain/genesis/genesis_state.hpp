#pragma once

namespace scorum { namespace chain { 

  struct genesis_account {
    string                    name;
    public_key_type           public_key;
    uint64_t                  scr_amount;
    uint64_t                  sp_amount;;
  };

  struct genesis_witness {
    string name;
    uint64_t votes;
  };

  struct genesis_state {
    vector<genesis_account> accounts;
  };

} }

FC_REFLECT( scorum::chain::genesis_account, (name)(public_key)(scr_amount)(sp_amount) )
FC_REFLECT( scorum::chain::genesis_state, (accounts) )