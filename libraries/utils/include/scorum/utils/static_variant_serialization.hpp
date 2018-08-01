#pragma once
#include <fc/variant.hpp>
#include <fc/reflect/variant.hpp>

namespace scorum {
namespace utils {

template <typename TStaticVariant> struct static_variant_convertor
{
    template <typename TVariantItem> void to_variant(const TVariantItem& obj, fc::variant& variant) const
    {
        fc::if_enum<typename fc::reflector<TVariantItem>::is_enum>::to_variant(obj, variant);
    }
    template <typename TVariantItem> void from_variant(const fc::variant& variant, TVariantItem& obj) const
    {
        fc::if_enum<typename fc::reflector<TVariantItem>::is_enum>::from_variant(variant, obj);
    }
    template <typename TVariantItem> std::string get_type_name(const TVariantItem& obj) const
    {
        std::string type_name = fc::get_typename<TVariantItem>::name();
        auto start = type_name.find_last_of(':') + 1;
        return type_name.substr(start, type_name.size() - start);
    }
};

template <typename TStaticVariant, typename TConvertor = static_variant_convertor<TStaticVariant>>
void to_variant(const TStaticVariant& static_var, fc::variant& var)
{
    TConvertor convertor;
    var = static_var.visit([&](const auto& obj) {
        fc::variant obj_as_var;
        convertor.to_variant(obj, obj_as_var);
        return fc::variant(std::make_pair(convertor.get_type_name(obj), std::move(obj_as_var)));
    });
}

template <typename TStaticVariant, typename TConvertor = static_variant_convertor<TStaticVariant>>
void from_variant(const fc::variant& var, TStaticVariant& static_var)
{
    TConvertor convertor;

    static std::map<std::string, uint32_t> to_tag = [&]() {
        std::map<std::string, uint32_t> name_map;
        for (int i = 0; i < std::decay_t<TStaticVariant>::count(); ++i)
        {
            TStaticVariant tmp;
            tmp.set_which(i);
            auto name = tmp.visit([&](const auto& v) { return convertor.get_type_name(v); });
            name_map[name] = i;
        }
        return name_map;
    }();

    auto var_tokens = var.get_array();
    if (var_tokens.size() < 2)
        return;

    if (var_tokens[0].is_uint64())
        static_var.set_which(var_tokens[0].as_uint64());
    else
    {
        auto itr = to_tag.find(var_tokens[0].as_string());
        FC_ASSERT(itr != to_tag.end(), "Invalid type name: ${n}", ("n", var_tokens[0]));
        static_var.set_which(to_tag[var_tokens[0].as_string()]);
    }

    static_var.visit([&](auto& v) { convertor.from_variant(var_tokens[1], v); });
}
}
}