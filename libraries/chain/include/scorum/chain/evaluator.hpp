#pragma once
#include <scorum/protocol/exceptions.hpp>
#include <scorum/protocol/operations.hpp>

namespace scorum {
namespace chain {

class data_service_factory_i;
class dbservice;

template <typename OperationType = scorum::protocol::operation> class evaluator
{
public:
    virtual void apply(const OperationType& op) = 0;
    virtual int get_type() const = 0;
};

template <typename DataService, typename EvaluatorType, typename OperationType = scorum::protocol::operation>
class evaluator_impl : public evaluator<OperationType>
{
public:
    evaluator_impl(DataService& d)
        : _db(d)
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

    DataService& db()
    {
        return _db;
    }

protected:
    DataService& _db;
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

#define DEFINE_EVALUATOR_DEPRECATED(X)                                                                                 \
    class X##_evaluator : public scorum::chain::evaluator_impl<dbservice, X##_evaluator>                               \
    {                                                                                                                  \
    public:                                                                                                            \
        typedef X##_operation operation_type;                                                                          \
                                                                                                                       \
        X##_evaluator(dbservice& db)                                                                                   \
            : scorum::chain::evaluator_impl<dbservice, X##_evaluator>(db)                                              \
        {                                                                                                              \
        }                                                                                                              \
                                                                                                                       \
        void do_apply(const X##_operation& o);                                                                         \
    };
