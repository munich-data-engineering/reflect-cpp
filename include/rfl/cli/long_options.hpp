#ifndef RFL_CLI_LONG_OPTIONS_HPP_
#define RFL_CLI_LONG_OPTIONS_HPP_

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Tuple.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/has_reflector.hpp"
#include "../internal/is_extra_fields.hpp"
#include "../internal/is_positional.hpp"
#include "../internal/is_short.hpp"
#include "../internal/no_extra_fields_v.hpp"
#include "../internal/processed_t.hpp"
#include "../parsing/is_string_map.hpp"
#include "Parser.hpp"
#include "Reader.hpp"

namespace rfl::cli {

/// What a long option does with the token that follows it.
enum class LongOptionKind {
  /// A boolean field: `--flag`. Only consumes a following `true`, `false`,
  /// `1` or `0`, like a short bool flag does.
  flag,
  /// A field that takes a value: `--key value` or `--key=value`.
  value,
  /// A known path whose kind cannot be decided from the type (a nested
  /// struct, or a sub-key of a variant). Never consumes the next token.
  other,
  /// No field matches this option.
  unknown
};

/// The long options that a target struct accepts, derived from its fields at
/// compile time. Lets parse_argv decide whether `--key` takes the next token
/// as its value. Paths are dotted (`database.host`) and use the field names
/// after processors (kebab-case, rfl::Rename) are applied.
struct LongOptions {
  /// Paths of boolean fields.
  std::set<std::string> flags;

  /// Paths of fields that take a value.
  std::set<std::string> values;

  /// Prefixes (ending in path_separator, or empty for the root) under which
  /// the keys are not known in advance, such as maps and rfl::ExtraFields.
  /// Maps to the kind of a direct sub-key.
  std::map<std::string, LongOptionKind> open_prefixes;

  /// Whether options that match no field are an error (rfl::NoExtraFields).
  bool reject_unknown = false;

  /// Returns the kind of the long option `--_key`.
  LongOptionKind kind_of(const std::string& _key) const {
    if (flags.count(_key)) {
      return LongOptionKind::flag;
    }
    if (values.count(_key)) {
      return LongOptionKind::value;
    }

    // A key below a leaf field (for example a variant of structs): the
    // reader decides what it means.
    for (auto pos = _key.find(path_separator); pos != std::string::npos;
         pos = _key.find(path_separator, pos + 1)) {
      const auto parent = _key.substr(0, pos);
      if (flags.count(parent) || values.count(parent)) {
        return LongOptionKind::other;
      }
    }

    // A nested struct or map itself, such as `--database`.
    const auto prefix = _key + path_separator;
    if (has_prefix(flags, prefix) || has_prefix(values, prefix)
        || has_prefix(open_prefixes, prefix)) {
      return LongOptionKind::other;
    }

    // A key inside a map or a struct with rfl::ExtraFields. The longest
    // matching prefix wins.
    const std::pair<const std::string, LongOptionKind>* open = nullptr;
    for (const auto& p : open_prefixes) {
      if (_key.size() > p.first.size() && _key.starts_with(p.first)
          && (!open || p.first.size() > open->first.size())) {
        open = &p;
      }
    }
    if (open) {
      const auto rest = std::string_view(_key).substr(open->first.size());
      return rest.find(path_separator) == std::string_view::npos
                 ? open->second
                 : LongOptionKind::other;
    }

    return LongOptionKind::unknown;
  }

 private:
  template <class ContainerType>
  static bool has_prefix(
      const ContainerType& _container, const std::string& _prefix
  ) {
    const auto it = _container.lower_bound(_prefix);
    if (it == _container.end()) {
      return false;
    }
    if constexpr (std::is_same_v<ContainerType, std::set<std::string>>) {
      return it->starts_with(_prefix);
    }
    else {
      return it->first.starts_with(_prefix);
    }
  }
};

namespace internal {

template <class... Ts>
struct type_list {
  template <class T>
  static constexpr bool contains = (std::is_same_v<T, Ts> || ...);

  template <class T>
  using add = type_list<Ts..., T>;
};

/// Strips everything that does not change how a field is written on the
/// command line: optionals, pointers, rfl::Short, rfl::Positional,
/// rfl::Rename, rfl::DefaultVal, rfl::Skip, types with a ReflectionType
/// (rfl::Validator, rfl::Description, rfl::Literal, ...), reflectors and
/// custom parsers.
template <class T, class ProcessorsType>
auto unwrap_option_type() {
  using U = std::remove_cvref_t<T>;
  if constexpr (std::is_pointer_v<U>) {
    return unwrap_option_type<std::remove_pointer_t<U>, ProcessorsType>();
  }
  else if constexpr (parsing::is_optional_v<U>) {
    return unwrap_option_type<typename U::value_type, ProcessorsType>();
  }
  else if constexpr (parsing::is_unique_ptr_v<U>
                     || parsing::is_shared_ptr_v<U>) {
    return unwrap_option_type<typename U::element_type, ProcessorsType>();
  }
  else if constexpr (parsing::is_box_v<U>) {
    return unwrap_option_type<
        typename parsing::is_box<U>::element_type, ProcessorsType>();
  }
  else if constexpr (parsing::is_ref_v<U>) {
    return unwrap_option_type<
        typename parsing::is_ref<U>::element_type, ProcessorsType>();
  }
  else if constexpr (rfl::internal::is_short_v<U>
                     || rfl::internal::is_positional_v<U>
                     || parsing::is_rename_v<U>
                     || parsing::is_default_val_v<U>
                     || parsing::is_skip_v<U>) {
    return unwrap_option_type<typename U::Type, ProcessorsType>();
  }
  else if constexpr (rfl::internal::has_reflection_type_v<U>) {
    return unwrap_option_type<typename U::ReflectionType, ProcessorsType>();
  }
  else if constexpr (rfl::internal::has_read_reflector<U>) {
    return unwrap_option_type<
        typename Reflector<U>::ReflType, ProcessorsType>();
  }
  else if constexpr (requires {
                       typename Parser<U, ProcessorsType>::
                           CustomParserHelperStruct;
                     }) {
    return unwrap_option_type<
        typename Parser<U, ProcessorsType>::CustomParserHelperStruct,
        ProcessorsType>();
  }
  else {
    return std::type_identity<U>{};
  }
}

template <class T, class ProcessorsType>
using unwrapped_option_t =
    typename decltype(unwrap_option_type<T, ProcessorsType>())::type;

/// Whether the reader treats T as a struct whose fields are addressed as
/// `--field.sub-field`.
template <class T>
constexpr bool is_option_struct_v =
    std::is_class_v<T> && std::is_aggregate_v<T> && !parsing::is_array_v<T>
    && !parsing::is_rfl_array_v<T>;

template <class T, class ProcessorsType>
constexpr LongOptionKind sub_key_kind() {
  using U = unwrapped_option_t<T, ProcessorsType>;
  if constexpr (std::is_same_v<U, bool>) {
    return LongOptionKind::flag;
  }
  else if constexpr (parsing::is_string_map_v<U> || is_option_struct_v<U>) {
    return LongOptionKind::other;
  }
  else {
    return LongOptionKind::value;
  }
}

template <class T, class ProcessorsType, class VisitedType>
void collect_long_options(const std::string& _prefix, LongOptions* _options);

template <class T, class ProcessorsType, class VisitedType>
void add_long_option(const std::string& _path, LongOptions* _options) {
  using U = unwrapped_option_t<T, ProcessorsType>;
  if constexpr (std::is_same_v<U, bool>) {
    _options->flags.insert(_path);
  }
  else if constexpr (parsing::is_string_map_v<U>) {
    _options->open_prefixes[_path + path_separator] =
        sub_key_kind<typename U::mapped_type, ProcessorsType>();
  }
  else if constexpr (is_option_struct_v<U>) {
    if constexpr (VisitedType::template contains<U>) {
      // A recursive type: stop here and let the reader handle deeper keys.
      _options->open_prefixes[_path + path_separator] = LongOptionKind::other;
    }
    else {
      collect_long_options<U, ProcessorsType, VisitedType>(
          _path + path_separator, _options);
    }
  }
  else {
    _options->values.insert(_path);
  }
}

template <class FieldType, class ProcessorsType, class VisitedType>
void add_long_option_for_field(
    const std::string& _prefix, LongOptions* _options
) {
  using T = std::remove_cvref_t<typename FieldType::Type>;
  if constexpr (rfl::internal::is_extra_fields_v<T>) {
    _options->open_prefixes[_prefix] =
        sub_key_kind<typename T::mapped_type, ProcessorsType>();
  }
  else {
    add_long_option<T, ProcessorsType, VisitedType>(
        _prefix + std::string(FieldType::name()), _options);
  }
}

template <class T, class ProcessorsType, class VisitedType>
void collect_long_options(const std::string& _prefix, LongOptions* _options) {
  using NT = rfl::internal::processed_t<T, ProcessorsType>;
  using NextVisitedType = typename VisitedType::template add<T>;
  [&]<size_t... _is>(std::index_sequence<_is...>) {
    (add_long_option_for_field<
         rfl::tuple_element_t<_is, typename NT::Fields>, ProcessorsType,
         NextVisitedType>(_prefix, _options),
     ...);
  }(std::make_index_sequence<NT::size()>());
}

}  // namespace internal

/// Derives the long options that T accepts from its fields.
/// @tparam T The target struct type
/// @tparam ProcessorsType The processors applied to the struct
/// @return The long options, with reject_unknown set if ProcessorsType
/// contains rfl::NoExtraFields
template <class T, class ProcessorsType>
LongOptions make_long_options() {
  LongOptions options;
  options.reject_unknown =
      rfl::internal::no_extra_fields_v<ProcessorsType>;
  using U = internal::unwrapped_option_t<T, ProcessorsType>;
  if constexpr (internal::is_option_struct_v<U>) {
    internal::collect_long_options<U, ProcessorsType, internal::type_list<>>(
        "", &options);
  }
  else if constexpr (parsing::is_string_map_v<U>) {
    options.open_prefixes[""] =
        internal::sub_key_kind<typename U::mapped_type, ProcessorsType>();
  }
  else {
    options.open_prefixes[""] = LongOptionKind::other;
  }
  return options;
}

}  // namespace rfl::cli

#endif
