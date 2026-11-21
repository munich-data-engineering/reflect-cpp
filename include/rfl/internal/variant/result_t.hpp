#ifndef RFL_INTERNAL_VARIANT_RESULT_T_HPP_
#define RFL_INTERNAL_VARIANT_RESULT_T_HPP_

#include <type_traits>
#include <utility>

#include "../nth_element_t.hpp"

namespace rfl::internal::variant {

template <class F, class... AlternativeTypes>
using result_t = std::remove_cv_t<decltype(
    std::declval<std::remove_cvref_t<F>>()(
        std::declval<internal::nth_element_t<0, AlternativeTypes...>&>()))>;

}  // namespace rfl::internal::variant

#endif
