// AssetWalk.hpp - walks over a struct of asset slots with C++26 static reflection, the member names become the file paths.

#pragma once

#if !defined(__cpp_impl_reflection)
#error "this project needs C++26 static reflection (GCC 16 with -freflection)"
#endif

#include <meta>
#include <string>
#include <type_traits>

namespace ljn {

//calls fn(path, slot) for every member of type Slot, also inside nested structs
//the path is the member names joined with '/', for example sprites/player/idle (no file extension)
//folder is what comes before the first name: empty, or text that ends with '/'
template <typename Slot, typename T, typename Fn>
void forEachAsset(T& value, const std::string& folder, Fn&& fn) {
    using Type = std::remove_cvref_t<T>;

    template for (constexpr auto member : std::define_static_array(
                      std::meta::nonstatic_data_members_of(^^Type, std::meta::access_context::unchecked()))) {
        using Member = std::remove_cvref_t<decltype(value.[:member:])>;
        const std::string path = folder + std::string(std::meta::identifier_of(member));

        if constexpr (std::is_same_v<Member, Slot>) {
            //a slot: this is the end of the path
            fn(path, value.[:member:]);
        } else if constexpr (std::is_aggregate_v<Member>) {
            //a group of slots: go inside it, its name becomes a folder
            forEachAsset<Slot>(value.[:member:], path + "/", fn);
        }
    }
}

}  //namespace ljn
