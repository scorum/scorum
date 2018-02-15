#include <scorum/chain/genesis/initializators/initializators.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

initializator_context::initializator_context(data_service_factory_i& _services,
                                             const genesis_state_type& _genesis_state)
    : services(_services)
    , genesis_state(_genesis_state)
{
}

initializator& initializator::after(initializator& impl)
{
    _after.push_back(std::ref(impl));
    return *this;
}

namespace {
std::string get_short_name(const std::string& long_name)
{
    static const std::string ns_separator("::");
    std::string ret(long_name);
    std::size_t pos = ret.rfind(ns_separator);
    if (pos != std::string::npos)
    {
        ret = ret.substr(pos + ns_separator.length());
    }
    pos = ret.rfind('_');
    if (pos != std::string::npos)
    {
        ret = ret.substr(0, pos);
    }
    return ret;
}
}

void initializator::apply(initializator_context& ctx)
{
    if (_applied)
        return;

    _applied = true;

    for (initializator& r : _after)
    {
        r.apply(ctx);
    }

    auto impl = get_short_name(boost::typeindex::type_id_runtime(*this).pretty_name());

    dlog("Genesis ${impl} is processing.", ("impl", impl));

    on_apply(ctx);

    dlog("Genesis ${impl} is done.", ("impl", impl));
}
}
}
}
