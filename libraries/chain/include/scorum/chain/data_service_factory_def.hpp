#pragma once

#include <boost/preprocessor/seq/for_each.hpp>

namespace scorum {
namespace chain {
namespace dba {
struct db_accessor_factory;
}
}
}

#define DECLARE_SERVICE_FUNCT_NAME(service) BOOST_PP_CAT(service, _service)

#define DECLARE_SERVICE_INTERFACE_NAME(service) BOOST_PP_CAT(DECLARE_SERVICE_FUNCT_NAME(service), _i)

#define DECLARE_DBS_IMPL_NAME(service) BOOST_PP_CAT(dbs_, service)

#define DECLARE_SERVICE_INTERFACE(_1, _2, service)                                                                     \
    struct DECLARE_SERVICE_INTERFACE_NAME(service);                                                                    \
    struct account_service_i;                                                                                          \
    struct witness_service_i;

#define DECLARE_FACTORY_INTERFACE_METHOD(_1, _2, service)                                                              \
    virtual DECLARE_SERVICE_INTERFACE_NAME(service)                                                                    \
        & BOOST_PP_CAT(DECLARE_SERVICE_FUNCT_NAME(service), BOOST_PP_EMPTY())() const                                  \
        = 0;

#define DECLARE_FACTORY_METHOD(_1, _2, service)                                                                        \
    virtual DECLARE_SERVICE_INTERFACE_NAME(service)                                                                    \
        & BOOST_PP_CAT(DECLARE_SERVICE_FUNCT_NAME(service), BOOST_PP_EMPTY())() const;

#define DATA_SERVICE_FACTORY_DECLARE(SERVICES)                                                                         \
    namespace scorum {                                                                                                 \
    namespace chain {                                                                                                  \
                                                                                                                       \
    class database;                                                                                                    \
    class dbservice_dbs_factory;                                                                                       \
                                                                                                                       \
    BOOST_PP_SEQ_FOR_EACH(DECLARE_SERVICE_INTERFACE, _, SERVICES)                                                      \
                                                                                                                       \
    struct data_service_factory_i                                                                                      \
    {                                                                                                                  \
        BOOST_PP_SEQ_FOR_EACH(DECLARE_FACTORY_INTERFACE_METHOD, _, SERVICES)                                           \
        virtual account_service_i& account_service() const = 0;                                                        \
        virtual witness_service_i& witness_service() const = 0;                                                        \
    };                                                                                                                 \
    class data_service_factory : public data_service_factory_i                                                         \
    {                                                                                                                  \
    public:                                                                                                            \
        explicit data_service_factory(scorum::chain::database& db);                                                    \
                                                                                                                       \
        virtual ~data_service_factory();                                                                               \
                                                                                                                       \
        BOOST_PP_SEQ_FOR_EACH(DECLARE_FACTORY_METHOD, _, SERVICES)                                                     \
        virtual account_service_i& account_service() const override;                                                   \
        virtual witness_service_i& witness_service() const override;                                                   \
                                                                                                                       \
    private:                                                                                                           \
        scorum::chain::dbservice_dbs_factory& factory;                                                                 \
        scorum::chain::dba::db_accessor_factory& dba_factory;                                                          \
    };                                                                                                                 \
    }                                                                                                                  \
    }

#define DECLARE_FACTORY_METHOD_IMPL(_1, _2, service)                                                                   \
    DECLARE_SERVICE_INTERFACE_NAME(service)                                                                            \
    &data_service_factory::BOOST_PP_CAT(DECLARE_SERVICE_FUNCT_NAME(service), BOOST_PP_EMPTY())() const                 \
    {                                                                                                                  \
        return factory.obtain_service<DECLARE_DBS_IMPL_NAME(service)>();                                               \
    }

#define DATA_SERVICE_FACTORY_IMPL(SERVICES)                                                                            \
    namespace scorum {                                                                                                 \
    namespace chain {                                                                                                  \
    data_service_factory::data_service_factory(scorum::chain::database& db)                                            \
        : factory(db)                                                                                                  \
        , dba_factory(db)                                                                                              \
    {                                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    data_service_factory::~data_service_factory()                                                                      \
    {                                                                                                                  \
    }                                                                                                                  \
    BOOST_PP_SEQ_FOR_EACH(DECLARE_FACTORY_METHOD_IMPL, _, SERVICES)                                                    \
    account_service_i& data_service_factory::account_service() const                                                   \
    {                                                                                                                  \
        return factory.obtain_service_explicit<dbs_account>(dynamic_global_property_service(), witness_service());     \
    }                                                                                                                  \
    witness_service_i& data_service_factory::witness_service() const                                                   \
    {                                                                                                                  \
        return factory.obtain_service_explicit<dbs_witness>(witness_schedule_service(),                                \
                                                            dynamic_global_property_service(),                         \
                                                            dba_factory.get_dba<chain_property_object>());             \
    }                                                                                                                  \
    }                                                                                                                  \
    }
