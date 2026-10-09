// JsonReflect.hpp - turns any simple struct into json and back, using C++26 static reflection.

#pragma once

#if !defined(__cpp_impl_reflection)
#error "this project needs C++26 static reflection (GCC 16 with -freflection)"
#endif

#include <meta>
#include <nlohmann/json.hpp>
#include <string>
#include <type_traits>

namespace ljn {

//writes every data member of a struct as a json field named after the member
template <typename T>
    requires std::is_aggregate_v<T>
nlohmann::json toJsonObject(const T& value) {
    nlohmann::json result;
    template for (constexpr auto member : std::define_static_array(
                      std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
        result[std::string(std::meta::identifier_of(member))] = value.[:member:];
    }
    return result;
}

//reads every data member of a struct from the json field with the same name
//throws a nlohmann json exception if a field is missing or has the wrong type
template <typename T>
    requires std::is_aggregate_v<T>
T fromJsonObject(const nlohmann::json& value) {
    T result{};
    template for (constexpr auto member : std::define_static_array(
                      std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()))) {
        value.at(std::string(std::meta::identifier_of(member))).get_to(result.[:member:]);
    }
    return result;
}

}  //namespace ljn
