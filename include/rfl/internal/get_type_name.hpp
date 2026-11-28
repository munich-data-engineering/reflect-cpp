#ifndef RFL_INTERNAL_GETTYPENAME_HPP_
#define RFL_INTERNAL_GETTYPENAME_HPP_

#ifndef REFLECTCPP_USE_CPP26_REFLECTION
#include "cpp20/get_type_name.hpp"
#else
#include "cpp26/get_type_name.hpp"
#endif

namespace rfl::internal {

template <class T>
consteval auto get_type_name() {
#ifndef REFLECTCPP_USE_CPP26_REFLECTION
  return cpp20::get_type_name<T>();
#else
  return cpp26::get_type_name<T>();
#endif
}

template <class T>
consteval auto get_type_name_str_view() {
#ifndef REFLECTCPP_USE_CPP26_REFLECTION
  return cpp20::get_type_name_str_view<T>();
#else
  return cpp26::get_type_name_str_view<T>();
#endif
}

}  // namespace rfl::internal

#endif
