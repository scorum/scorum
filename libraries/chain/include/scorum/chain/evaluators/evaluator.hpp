#pragma once
#include <scorum/protocol/exceptions.hpp>
#include <scorum/protocol/operations.hpp>

namespace scorum {
namespace chain {

class data_service_factory_i;
class database;

template <typename OperationType = scorum::protocol::operation> class evaluator
{
public:
    virtual void apply(const OperationType& op) = 0;
    virtual int get_type() const = 0;
};

template <typename DataServices, typename EvaluatorType, typename OperationType = scorum::protocol::operation>
class evaluator_impl : public evaluator<OperationType>
{
public:
    evaluator_impl(DataServices& d)
        : _services(d)
    {
    }

    virtual void apply(const OperationType& o) final override
    {
        auto* eval = static_cast<EvaluatorType*>(this);
        const auto& op = o.template get<typename EvaluatorType::operation_type>();
        eval->do_apply(op);
    }

    virtual int get_type() const override
    {
        return OperationType::template tag<typename EvaluatorType::operation_type>::value;
    }

    DataServices& db()
    {
        return _services;
    }

private:
    DataServices& _services;
};

} // namespace scorum
} // namespace chain

#define DEFINE_EVALUATOR(X)                                                                                            \
    class X##_evaluator : public scorum::chain::evaluator_impl<data_service_factory_i, X##_evaluator>                  \
    {                                                                                                                  \
    public:                                                                                                            \
        typedef X##_operation operation_type;                                                                          \
                                                                                                                       \
        X##_evaluator(data_service_factory_i& db)                                                                      \
            : scorum::chain::evaluator_impl<data_service_factory_i, X##_evaluator>(db)                                 \
        {                                                                                                              \
        }                                                                                                              \
                                                                                                                       \
        void do_apply(const X##_operation& o);                                                                         \
    };
