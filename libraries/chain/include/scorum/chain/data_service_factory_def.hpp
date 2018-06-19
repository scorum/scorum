#pragma once

#include <boost/preprocessor/seq/for_each.hpp>

#define DECLARE_SERVICE_FUNCT_NAME(service) BOOST_PP_CAT(service, _service)

#define DECLARE_SERVICE_INTERFACE_NAME(service) BOOST_PP_CAT(DECLARE_SERVICE_FUNCT_NAME(service), _i)

#define DECLARE_DBS_IMPL_NAME(service) BOOST_PP_CAT(dbs_, service)

#define DECLARE_SERVICE_INTERFACE(_1, _2, service) struct DECLARE_SERVICE_INTERFACE_NAME(service);

#define DECLARE_FACTORY_INTERFACE_METHOD(_1, _2, service)                                                              \
    virtual DECLARE_SERVICE_INTERFACE_NAME(service)                                                                    \
        & BOOST_PP_CAT(DECLARE_SERVICE_FUNCT_NAME(service), BOOST_PP_EMPTY())()                                        \
        = 0;

#define DECLARE_FACTORY_METHOD(_1, _2, service)                                                                        \
    virtual DECLARE_SERVICE_INTERFACE_NAME(service)                                                                    \
        & BOOST_PP_CAT(DECLARE_SERVICE_FUNCT_NAME(service), BOOST_PP_EMPTY())();

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
    };                                                                                                                 \
    class data_service_factory : public data_service_factory_i                                                         \
    {                                                                                                                  \
    public:                                                                                                            \
        explicit data_service_factory(scorum::chain::database& db);                                                    \
                                                                                                                       \
        virtual ~data_service_factory();                                                                               \
                                                                                                                       \
        BOOST_PP_SEQ_FOR_EACH(DECLARE_FACTORY_METHOD, _, SERVICES)                                                     \
                                                                                                                       \
    private:                                                                                                           \
        scorum::chain::dbservice_dbs_factory& factory;                                                                 \
    };                                                                                                                 \
    }                                                                                                                  \
    }

#define DECLARE_FACTORY_METHOD_IMPL(_1, _2, service)                                                                   \
    DECLARE_SERVICE_INTERFACE_NAME(service)                                                                            \
    &data_service_factory::BOOST_PP_CAT(DECLARE_SERVICE_FUNCT_NAME(service), BOOST_PP_EMPTY())()                       \
    {                                                                                                                  \
        return static_cast<DECLARE_SERVICE_INTERFACE_NAME(service)&>(                                                  \
            factory.obtain_service<DECLARE_DBS_IMPL_NAME(service)>());                                                 \
    }

#define DATA_SERVICE_FACTORY_IMPL(SERVICES)                                                                            \
    namespace scorum {                                                                                                 \
    namespace chain {                                                                                                  \
    data_service_factory::data_service_factory(scorum::chain::database& db)                                            \
        : factory(db)                                                                                                  \
    {                                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    data_service_factory::~data_service_factory()                                                                      \
    {                                                                                                                  \
    }                                                                                                                  \
    BOOST_PP_SEQ_FOR_EACH(DECLARE_FACTORY_METHOD_IMPL, _, SERVICES)                                                    \
    }                                                                                                                  \
    }
