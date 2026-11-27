#ifndef RFL_INTERNAL_ENUMS_CPP26_FROM_STRING_HPP_
#define RFL_INTERNAL_ENUMS_CPP26_FROM_STRING_HPP_

#ifdef REFLECTCPP_USE_CPP26_REFLECTION

#include <meta>
#include <optional>
#include <string_view>
#include <type_traits>

namespace rfl::internal::enums::cpp26 {

template <class EnumType>
std::optional<EnumType> from_string(const std::string_view _str) {
  template for (constexpr auto e : std::define_static_array(
                    std::meta::enumerators_of(^^EnumType))) {
    if (_str == std::meta::identifier_of(e)) {
      return [:e:];
    }
  }
  return std::nullopt;
}

}  // namespace rfl::internal::enums::cpp26

#endif
#endif
