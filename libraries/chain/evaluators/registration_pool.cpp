#include <scorum/chain/evaluators/registration_pool.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/registration_committee.hpp>

namespace scorum {
namespace chain {

class registration_pool_impl
{
public:
    registration_pool_impl(data_service_factory_i& services)
        : _account_service(services.account_service())
        , _registration_pool_service(services.registration_pool_service())
        , _registration_committee_service(services.registration_committee_service())
    {
    }

    // TODO

private:
    account_service_i& _account_service;
    registration_pool_service_i& _registration_pool_service;
    registration_committee_service_i& _registration_committee_service;
};

//

registration_pool_evaluator::registration_pool_evaluator(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, registration_pool_evaluator>(services)
    , _impl(new registration_pool_impl(services))
    , _account_service(services.account_service())
    , _registration_pool_service(services.registration_pool_service())
    , _registration_committee_service(services.registration_committee_service())
{
}

registration_pool_evaluator::~registration_pool_evaluator()
{
}

void registration_pool_evaluator::do_apply(const registration_pool_evaluator::operation_type& o)
{
    _account_service.check_account_existence(o.creator);

    _account_service.check_account_existence(o.owner.account_auths);

    _account_service.check_account_existence(o.active.account_auths);

    _account_service.check_account_existence(o.posting.account_auths);

    FC_ASSERT(_registration_pool_service.is_exists(), "Registration pool is exhausted.");

    FC_ASSERT(_registration_committee_service.is_exists(o.creator), "Account '${1}' is not committee member.",
              ("1", o.creator));

    // TODO
}

//

registration_pool_context::registration_pool_context(data_service_factory_i& services)
    : _services(services)
{
}

void registration_pool_task::on_apply(registration_pool_context&)
{
    // TODO
}
}
}
