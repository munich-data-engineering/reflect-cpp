#ifndef RFL_INTERNAL_ENUMS_FROM_STRING_HPP_
#define RFL_INTERNAL_ENUMS_FROM_STRING_HPP_

#include <string_view>

#ifndef REFLECTCPP_USE_CPP26_REFLECTION
#include "cpp20/from_string.hpp"
#else
#include "cpp26/from_string.hpp"
#endif

namespace rfl::internal::enums {

template <class EnumType>
auto from_string(const std::string_view _str) {
#ifndef REFLECTCPP_USE_CPP26_REFLECTION
  return cpp20::from_string<EnumType>(_str);
#else
  return cpp26::from_string<EnumType>(_str);
#endif
}

}  // namespace rfl::internal::enums

#endif
