#ifndef RFL_PARSING_VARIANTALTERNATIVEWRAPPER_HPP_
#define RFL_PARSING_VARIANTALTERNATIVEWRAPPER_HPP_

#include <type_traits>
#include <utility>

#include "../Field.hpp"
#include "../Literal.hpp"
#include "../internal/StringLiteral.hpp"
#include "../internal/get_type_name.hpp"
#include "../internal/has_tag_v.hpp"
#include "../internal/remove_namespaces.hpp"

namespace rfl::parsing {

namespace vaw {

template <class T>
struct GetName {};

template <internal::StringLiteral _name>
struct GetName<Literal<_name>> {
  constexpr static internal::StringLiteral name_ = _name;
};

/**
 * @brief Makes a tag for the variant alternative.
 *
 * @tparam T The type to make the tag for.
 * @tparam _remove_namespaces Whether to remove namespaces from the name.
 * @return The tag for the type.
 */
template <class T, bool _remove_namespaces = true>
consteval auto make_tag() {
  if constexpr (internal::has_tag_v<T>) {
    return typename T::Tag();
  } else if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::string>) {
    return Literal<"std::string">();
  } else if constexpr (_remove_namespaces) {
    return Literal<
        internal::remove_namespaces<internal::get_type_name<T>()>()>();
  } else {
    return Literal<internal::get_type_name<T>()>();
  }
}

template <class T, bool _remove_namespaces = true>
using tag_t = decltype(make_tag<std::remove_cvref_t<std::remove_pointer_t<T>>,
                                _remove_namespaces>());

}  // namespace vaw

template <class T, bool _remove_namespaces = true>
using VariantAlternativeWrapper = Field<vaw::GetName<vaw::tag_t<T, _remove_namespaces>>::name_, T>;

}  // namespace rfl::parsing

#endif
