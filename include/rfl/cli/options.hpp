#ifndef RFL_CLI_OPTIONS_HPP_
#define RFL_CLI_OPTIONS_HPP_

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../Positional.hpp"
#include "../Short.hpp"
#include "../Tuple.hpp"
#include "../internal/StringLiteral.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/has_reflector.hpp"
#include "../internal/is_extra_fields.hpp"
#include "../internal/is_named_tuple.hpp"
#include "../internal/is_positional.hpp"
#include "../internal/is_short.hpp"
#include "../internal/processed_t.hpp"
#include "../parsing/is_string_map.hpp"
#include "Parser.hpp"
#include "Reader.hpp"

namespace rfl::cli::internal {

/// What an option does with the token that follows it.
enum class OptionKind {
  /// A boolean field: `--flag` or `-f`. `--flag` never takes the next
  /// token; `-f` takes it only if it is `true`, `false`, `1` or `0`.
  flag,
  /// A field that takes a value: `--key=value`, `-k value` or `-k=value`.
  value,
  /// A known path whose kind cannot be decided from the type (a nested
  /// struct, a map, a variant or rfl::Generic). `--key`
  /// never takes the next token, so alone it passes an empty value. A short
  /// alias `-k` takes the next token unless it looks like an option.
  other,
  /// No field matches this option.
  unknown
};

/// How an option is given on the command line.
struct Option {
  OptionKind kind = OptionKind::unknown;
};

/// The options that a target struct accepts, built from its fields by
/// make_options on every read. parse_argv uses it to decide whether a short
/// option takes the next token as its value, and resolve_args to map short
/// aliases and positional arguments to fields. Names are the field names
/// after processors (kebab-case, rfl::Rename) are applied.
struct Options {
  /// The options of the top-level fields, by name.
  std::map<std::string, Option> long_options;

  /// The short aliases given by rfl::Short (`p` for `-p`), mapped to the path
  /// of their field.
  std::map<std::string, std::string> short_options;

  /// The paths of the rfl::Positional fields, in declaration order. Each
  /// takes one positional argument.
  std::vector<std::string> positionals;

  /// Whether tokens such as `-5` and `-.5` are short options rather than
  /// negative numbers. They are when parse_argv is called without the
  /// struct. make_options clears it unless, as in argparse, a short option's
  /// name looks like a number (`-1`).
  bool negative_numbers_are_options = true;

  /// A short name that two fields give with rfl::Short, such as
  /// `rfl::Short<"o", A> a; rfl::Short<"o", B> b;`, or empty. Reading such a
  /// struct fails.
  std::string duplicate_short_name;

  /// Returns the long option `--_name`.
  Option find_long(const std::string& _name) const {
    const auto it = long_options.find(_name);
    return it == long_options.end() ? Option{} : it->second;
  }

  /// Returns the option that the short alias `-_name` stands for.
  Option find_short(const std::string& _name) const {
    const auto it = short_options.find(_name);
    return it == short_options.end() ? Option{} : find_long(it->second);
  }
};

/// Returns true if the token is one of the values that a bool flag given
/// without '=' takes from the next token: "true", "false", "1" or "0".
/// @param _token The command-line token to check
/// @return true if it is a boolean literal, false otherwise
inline bool is_bool_literal(std::string_view _token) noexcept {
  return _token == "true" || _token == "false" || _token == "1"
         || _token == "0";
}

/// Whether T is rfl::Short directly around rfl::Positional, or the other way
/// round, which is a compile-time error.
template <class T>
consteval bool is_short_around_positional() {
  if constexpr (rfl::internal::is_short_v<T>) {
    return rfl::internal::is_positional_v<typename T::Type>;
  }
  else if constexpr (rfl::internal::is_positional_v<T>) {
    return rfl::internal::is_short_v<typename T::Type>;
  }
  else {
    return false;
  }
}

/// The CLI metadata of the _i-th field of a NamedTuple. InnerType is the
/// declared type of the field, whichever wrappers it consists of.
template <class NamedTupleType, size_t _i>
struct field_info {
  using FieldType = rfl::tuple_element_t<_i, typename NamedTupleType::Fields>;
  using InnerType = typename FieldType::Type;
  using OuterType = std::remove_pointer_t<std::remove_cvref_t<InnerType>>;

  /// Whether rfl::Positional is the outermost wrapper of the field.
  static constexpr bool is_positional =
      rfl::internal::is_positional_v<OuterType>;
  /// Whether rfl::Short is the outermost wrapper of the field.
  static constexpr bool is_short = rfl::internal::is_short_v<OuterType>;

  static_assert(
      !is_short_around_positional<OuterType>(),
      "Nested wrappers (Positional<Short<...>> or Short<..., Positional<...>>) "
      "are not allowed.");

  static constexpr std::string_view name() { return FieldType::name(); }
};

/// The short name of the _i-th field, or an empty string if it has none.
template <class NamedTupleType, size_t _i>
constexpr auto get_short_name() {
  using Info = field_info<NamedTupleType, _i>;
  if constexpr (Info::is_short) {
    return Info::OuterType::short_name_;
  }
  else {
    return rfl::internal::StringLiteral<1>("");
  }
}

/// Strips everything that does not change how a field is written on the
/// command line: optionals, pointers, rfl::Short, rfl::Positional,
/// rfl::Rename, rfl::DefaultVal, rfl::Skip, types with a
/// ReflectionType (rfl::Validator, rfl::Description, rfl::Literal, ...),
/// reflectors and custom parsers.
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
  else if constexpr (rfl::internal::has_read_reflector<U>) {
    return unwrap_option_type<
        typename Reflector<U>::ReflType, ProcessorsType>();
  }
  else if constexpr (rfl::internal::has_reflection_type_v<U>) {
    return unwrap_option_type<typename U::ReflectionType, ProcessorsType>();
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
    (std::is_class_v<T> && std::is_aggregate_v<T> && !parsing::is_array_v<T>
     && !parsing::is_rfl_array_v<T>)
    || rfl::internal::is_named_tuple_v<T>;

/// Whether an option for T may or may not need a value, depending on what
/// it holds: a variant or rfl::Generic can be a bool or a struct. Such an
/// option never takes the next token.
template <class T>
constexpr bool is_open_option_v =
    parsing::is_variant_v<T> || parsing::is_rfl_variant_v<T>
    || parsing::is_tagged_union_v<T>;

/// The NamedTuple whose fields are the options of the struct T: T itself if
/// it is an rfl::NamedTuple (whose names the processors leave alone), or T
/// with the processors applied.
template <class T, class ProcessorsType>
using option_fields_t = typename std::conditional_t<
    rfl::internal::is_named_tuple_v<T>, std::type_identity<T>,
    rfl::internal::Processed<T, ProcessorsType>>::type;

/// Adds the option of the top-level field `_name` of type T.
template <class T, class ProcessorsType>
void add_option(const std::string& _name, Options* _options) {
  using U = unwrapped_option_t<T, ProcessorsType>;
  if constexpr (std::is_same_v<U, bool>) {
    _options->long_options[_name] = Option{OptionKind::flag};
  }
  else if constexpr (is_open_option_v<U> || parsing::is_string_map_v<U>
                     || is_option_struct_v<U>) {
    _options->long_options[_name] = Option{OptionKind::other};
  }
  else {
    _options->long_options[_name] = Option{OptionKind::value};
  }
}

template <class FieldType, class ProcessorsType>
void add_option_for_field(Options* _options) {
  using T = std::remove_cvref_t<typename FieldType::Type>;
  if constexpr (!rfl::internal::is_extra_fields_v<T>) {
    add_option<T, ProcessorsType>(std::string(FieldType::name()), _options);
  }
}

/// Adds the options of the top-level fields of T.
template <class T, class ProcessorsType>
void collect_options(Options* _options) {
  using NT = option_fields_t<T, ProcessorsType>;
  [&]<size_t... _is>(std::index_sequence<_is...>) {
    (add_option_for_field<rfl::tuple_element_t<_is, typename NT::Fields>,
                          ProcessorsType>(_options),
     ...);
  }(std::make_index_sequence<NT::size()>());
}

/// Adds the short alias and the positional slot of the _i-th top-level
/// field, if it has them. Two fields with the same short name are kept as
/// an error (Options::duplicate_short_name).
template <class NamedTupleType, size_t _i>
void add_short_and_positional(Options* _options) {
  using Info = field_info<NamedTupleType, _i>;
  if constexpr (Info::is_short) {
    const auto name =
        std::string(get_short_name<NamedTupleType, _i>().string_view());
    if (!_options->short_options.emplace(name, std::string(Info::name()))
             .second
        && _options->duplicate_short_name.empty()) {
      _options->duplicate_short_name = name;
    }
  }
  if constexpr (Info::is_positional) {
    _options->positionals.emplace_back(Info::name());
  }
}

template <class T, class ProcessorsType>
void collect_short_and_positional(Options* _options) {
  using NT = option_fields_t<T, ProcessorsType>;
  [&]<size_t... _is>(std::index_sequence<_is...>) {
    (add_short_and_positional<NT, _is>(_options), ...);
  }(std::make_index_sequence<NT::size()>());
}

/// Derives the options that T accepts from its fields: the option of every
/// top-level field, with whether it is a flag or takes a value, and the
/// rfl::Short aliases and rfl::Positional fields.
/// @tparam T The target struct type
/// @tparam ProcessorsType The processors applied to the struct
/// @return The options
template <class T, class ProcessorsType>
Options make_options() {
  Options options;
  using U = unwrapped_option_t<T, ProcessorsType>;
  if constexpr (is_option_struct_v<U>) {
    collect_options<U, ProcessorsType>(&options);
    collect_short_and_positional<U, ProcessorsType>(&options);
  }
  // As in argparse, -5 is a negative number unless a short option's name
  // looks like a number (-1).
  options.negative_numbers_are_options = false;
  for (const auto& [name, path] : options.short_options) {
    if (!name.empty()
        && ((name[0] >= '0' && name[0] <= '9') || name[0] == '.')) {
      options.negative_numbers_are_options = true;
    }
  }
  return options;
}

}  // namespace rfl::cli::internal

#endif
