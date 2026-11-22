#ifndef RFL_INTERNAL_TAG_T_HPP_
#define RFL_INTERNAL_TAG_T_HPP_

#include <type_traits>
#include <utility>

#include "StringLiteral.hpp"
#include "make_tag.hpp"

namespace rfl::internal {

template <internal::StringLiteral _discriminator, class T>
using tag_t = decltype(make_tag<_discriminator, T>(std::declval<T>()));

}  // namespace rfl::internal

#endif
