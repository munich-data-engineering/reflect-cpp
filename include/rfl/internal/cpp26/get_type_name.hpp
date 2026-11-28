#ifndef RFL_INTERNAL_CPP26_GETTYPENAME_HPP_
#define RFL_INTERNAL_CPP26_GETTYPENAME_HPP_

#ifdef REFLECTCPP_USE_CPP26_REFLECTION

#include <meta>
#include <type_traits>

#include "../StringLiteral.hpp"

namespace rfl::internal::cpp26 {

template <class T>
consteval auto get_type_name_str_view() {
  using Type = std::remove_cvref_t<T>;
  if constexpr (std::is_pointer_v<Type>) {
    return get_type_name_str_view<std::remove_pointer_t<Type>>();
  } else {
    // TODO: Is there a way to get the name of the type without using
    // compiler-specific extensions?
#if defined(__GNUC__)
    constexpr auto display_string = std::meta::display_string_of(^^Type);
    constexpr auto drop_last_char =
        display_string.substr(0, display_string.size() - 1);
    return drop_last_char.substr(drop_last_char.find("{aka ") + 5);
#else
    static_assert(false,
                  "You are using an unsupported compiler. Please use GCC or "
                  "explicitly tag your structs using 'Tag' or 'Name'.");
#endif
  }
}

template <class T>
consteval auto get_type_name() {
  constexpr auto name = get_type_name_str_view<T>();
  return StringLiteral<name.size() + 1>(name);
}
}  // namespace rfl::internal::cpp26

#endif
#endif
