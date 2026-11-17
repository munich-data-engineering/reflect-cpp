#ifndef RFL_PARSING_CHECK_DEFAULT_VALUE_HPP_
#define RFL_PARSING_CHECK_DEFAULT_VALUE_HPP_

#include <atomic>
#include <cstddef>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <variant>

#include "../Generic.hpp"
#include "../Result.hpp"
#include "../apply.hpp"
#include "../internal/has_reflection_method_v.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/has_reflector.hpp"
#include "../internal/is_bitset.hpp"
#include "../internal/is_default_val_v.hpp"
#include "../internal/is_deprecated.hpp"
#include "../internal/is_description.hpp"
#include "../internal/is_field.hpp"
#include "../internal/is_flatten_field.hpp"
#include "../internal/is_literal.hpp"
#include "../internal/is_named_tuple.hpp"
#include "../internal/is_validator.hpp"
#include "../num_fields.hpp"
#include "../to_view.hpp"
#include "../type_name_t.hpp"
#include "ParserAtomic.hpp"
#include "ParserBox.hpp"
#include "ParserCommented.hpp"
#include "ParserDuration.hpp"
#include "ParserExpected.hpp"
#include "ParserFilepath.hpp"
#include "ParserOptional.hpp"
#include "ParserPair.hpp"
#include "ParserPositional.hpp"
#include "ParserRef.hpp"
#include "ParserReferenceWrapper.hpp"
#include "ParserRename.hpp"
#include "ParserResult.hpp"
#include "ParserRflArray.hpp"
#include "ParserRflTuple.hpp"
#include "ParserRflVariant.hpp"
#include "ParserSharedPtr.hpp"
#include "ParserShort.hpp"
#include "ParserSkip.hpp"
#include "ParserTaggedUnion.hpp"
#include "ParserTimePoint.hpp"
#include "ParserTuple.hpp"
#include "ParserUniquePtr.hpp"
#include "ParserVariant.hpp"

namespace rfl::parsing {

/// What check_default_value has found in a default value.
struct DefaultValueCheck {
  /// Whether to stop at the first invalid Validator or unchecked type.
  bool stop_early = true;

  /// The number of Validators whose value is invalid.
  size_t num_invalid = 0;

  /// Why the first of them is invalid.
  std::optional<Error> invalid;

  /// The first type that we could not look into, so that we cannot tell
  /// whether it holds an invalid Validator.
  std::optional<std::string> unchecked;

  /// Visited pointees of raw/shared pointers to avoid cycles and double-counts.
  std::unordered_set<const void*> visited;

  bool done() const noexcept { return stop_early && (invalid || unchecked); }
};

/// Types that cannot hold an rfl::Validator, so there is no need to look
/// into them.
template <class U>
constexpr bool holds_no_validator_v =
    !std::is_pointer_v<U> &&
    (std::is_arithmetic_v<U> || std::is_enum_v<U> ||
     std::is_null_pointer_v<U> || std::is_member_pointer_v<U> ||
     std::is_same_v<U, std::string> || std::is_same_v<U, std::wstring> ||
     std::is_same_v<U, std::string_view> || std::is_same_v<U, Generic> ||
     std::is_same_v<U, std::atomic_flag> || std::is_same_v<U, std::monostate> ||
     is_duration_v<U> || is_time_point_v<U> || is_filepath_v<U> ||
     internal::is_literal_v<U> || internal::is_bitset_v<U> ||
     (std::is_empty_v<U> && !internal::is_validator_v<U>));

/// Whether U is a reflect-cpp wrapper whose inner value can be inspected via
/// .value() by const reference without copying.
template <class U>
constexpr bool is_value_wrapper_v =
    !std::is_pointer_v<U> &&
    (is_skip_v<U> || internal::is_default_val_v<U> || is_rename_v<U> ||
     internal::is_flatten_field_v<U> || is_short_v<U> || is_positional_v<U> ||
     internal::is_field_v<U> || internal::is_description_v<U> ||
     internal::is_deprecated_v<U> || is_commented_v<U>);

/// Looks for rfl::Validators whose value is invalid inside _value and records
/// them in _check. The parser uses it when a Validator has kept an invalid
/// default value (see internal::DeferredDefaults). It walks into structs,
/// containers, optionals, pointers, variants, tuples, the wrappers of
/// reflect-cpp, and classes with a reflection() method or an rfl::Reflector.
/// Any other type is recorded as unchecked, unless it cannot hold a Validator
/// at all (see holds_no_validator_v).
template <class T>
void check_default_value(const T& _value, DefaultValueCheck* _check) {
  using U = std::remove_cvref_t<T>;

  if (_check->done()) {
    return;
  }

  const auto in_each = [_check](const auto&... _vs) {
    (check_default_value(_vs, _check), ...);
  };

  const auto in_alternative = [_check](const auto& _v) {
    check_default_value(_v, _check);
  };

  if constexpr (std::is_pointer_v<U>) {
    using Pointee = std::remove_pointer_t<U>;
    if (!_value || std::is_function_v<Pointee>) {
      return;
    }
    if constexpr (std::is_void_v<Pointee>) {
      _check->unchecked = type_name_t<U>().str();
    } else if (_check->visited.insert(static_cast<const void*>(_value)).second) {
      check_default_value(*_value, _check);
    }

  } else if constexpr (internal::is_validator_v<U>) {
    const auto res = U::ValidationType::validate(_value.value());
    if (!res) {
      ++_check->num_invalid;
      if (!_check->invalid) {
        _check->invalid = res.error();
      }
    }
    check_default_value(_value.value(), _check);

  } else if constexpr (holds_no_validator_v<U>) {
    return;

  } else if constexpr (is_optional_v<U>) {
    if (_value) {
      check_default_value(*_value, _check);
    }

  } else if constexpr (is_unique_ptr_v<U>) {
    if (_value) {
      if constexpr (requires { *_value; } &&
                    !std::is_void_v<std::remove_cvref_t<decltype(*_value)>>) {
        check_default_value(*_value, _check);
      } else {
        _check->unchecked = type_name_t<U>().str();
      }
    }

  } else if constexpr (is_shared_ptr_v<U>) {
    if (_value) {
      if constexpr (requires { *_value; } &&
                    !std::is_void_v<std::remove_cvref_t<decltype(*_value)>>) {
        if (_check->visited.insert(static_cast<const void*>(_value.get()))
                .second) {
          check_default_value(*_value, _check);
        }
      } else {
        _check->unchecked = type_name_t<U>().str();
      }
    }

  } else if constexpr (is_box_v<U>) {
    if (_value.get()) {
      check_default_value(*_value, _check);
    }

  } else if constexpr (is_ref_v<U>) {
    if (_value.get() &&
        _check->visited.insert(static_cast<const void*>(_value.get())).second) {
      check_default_value(*_value, _check);
    }

  } else if constexpr (is_result_v<U> || is_expected_v<U>) {
    if (_value) {
      if constexpr (requires { *_value; } &&
                    !std::is_void_v<std::remove_cvref_t<decltype(*_value)>>) {
        check_default_value(*_value, _check);
      }
    } else if constexpr (requires { _value.error(); }) {
      check_default_value(_value.error(), _check);
    }

  } else if constexpr (is_reference_wrapper_v<U>) {
    check_default_value(_value.get(), _check);

  } else if constexpr (is_atomic_v<U>) {
    check_default_value(_value.load(), _check);

  } else if constexpr (is_variant_v<U>) {
    if (!_value.valueless_by_exception()) {
      std::visit(in_alternative, _value);
    }

  } else if constexpr (is_rfl_variant_v<U>) {
    _value.visit(in_alternative);

  } else if constexpr (is_tagged_union_v<U>) {
    _value.variant().visit(in_alternative);

  } else if constexpr (is_tuple_v<U> || is_pair_v<U>) {
    std::apply(in_each, _value);

  } else if constexpr (is_rfl_tuple_v<U>) {
    rfl::apply(in_each, _value);

  } else if constexpr (internal::is_named_tuple_v<U>) {
    rfl::apply(in_each, _value.values());

  } else if constexpr (is_rfl_array_v<U>) {
    check_default_value(_value.arr_, _check);

  } else if constexpr (is_value_wrapper_v<U>) {
    check_default_value(_value.value(), _check);

  } else if constexpr (std::ranges::range<const U>) {
    using Element = std::remove_cvref_t<std::ranges::range_value_t<const U>>;
    if constexpr (!holds_no_validator_v<Element>) {
      for (const auto& element : _value) {
        check_default_value(element, _check);
      }
    }

  } else if constexpr (internal::has_write_reflector<U> &&
                       requires(const U& _u) { Reflector<U>::from(_u); }) {
    check_default_value(Reflector<U>::from(_value), _check);

  } else if constexpr (internal::has_reflection_type_v<U> &&
                       internal::has_reflection_method_v<U> &&
                       requires(const U& _u) { _u.reflection(); }) {
    check_default_value(_value.reflection(), _check);

  } else if constexpr (!internal::has_read_reflector<U> &&
                       std::is_class_v<U> && std::is_aggregate_v<U>) {
    if constexpr (rfl::num_fields<U> <= 128) {
      rfl::apply([&](const auto&... _fields) { in_each(*_fields.value()...); },
                 to_view(_value).fields());
    } else {
      _check->unchecked = type_name_t<U>().str();
    }

  } else {
    _check->unchecked = type_name_t<U>().str();
  }
}

}  // namespace rfl::parsing

#endif
