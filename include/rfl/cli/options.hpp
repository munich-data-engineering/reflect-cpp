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

#include "../Box.hpp"
#include "../DefaultVal.hpp"
#include "../Deprecated.hpp"
#include "../Description.hpp"
#include "../Positional.hpp"
#include "../Rename.hpp"
#include "../Short.hpp"
#include "../Tuple.hpp"
#include "../internal/StringLiteral.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/has_reflector.hpp"
#include "../internal/is_extra_fields.hpp"
#include "../internal/is_named_tuple.hpp"
#include "../internal/is_positional.hpp"
#include "../internal/is_short.hpp"
#include "../internal/no_extra_fields_v.hpp"
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
  /// struct, a variant, rfl::Generic, or a sub-key of a variant). `--key`
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
/// option takes the next token as its value and whether a long option is
/// known, and resolve_args to map short aliases and positional arguments to
/// fields. Paths are dotted
/// (`database.host`) and use the field names after processors (kebab-case,
/// rfl::Rename) are applied.
struct Options {
  /// The long options, by path.
  std::map<std::string, Option> long_options;

  /// The short aliases given by rfl::Short (`p` for `-p`), mapped to the path
  /// of their field.
  std::map<std::string, std::string> short_options;

  /// The paths of the rfl::Positional fields, in declaration order, wherever
  /// rfl::Positional sits among a field's wrappers. Each takes one positional
  /// argument.
  std::vector<std::string> positionals;

  /// Whether short options can be grouped (`-qv`, `-fcsv`). make_options
  /// sets it. Without a table (the table-less parse_argv), `-abc` is the
  /// one short option `abc`.
  bool group_short_options = false;

  /// Prefixes (ending in path_separator, or empty for the root) under which
  /// the keys are not known in advance, such as maps and rfl::ExtraFields.
  /// Maps to the option of a direct sub-key.
  std::map<std::string, Option> open_prefixes;

  /// For the open prefixes whose sub-keys hold structs or maps: the options
  /// inside one such value, with paths relative to it, so that
  /// `--servers.eu.port` is looked up as `port`.
  std::map<std::string, rfl::Box<Options>> open_prefix_options;

  /// Whether options that match no field are an error (rfl::NoExtraFields).
  bool reject_unknown = false;

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
    if (it != long_options.end()) {
      return it->second;
    }

    // A key below an open leaf field (for example a variant of structs): the
    // reader decides what it means.
    for (auto pos = _name.find(path_separator); pos != std::string::npos;
         pos = _name.find(path_separator, pos + 1)) {
      const auto parent = long_options.find(_name.substr(0, pos));
      if (parent != long_options.end()
          && parent->second.kind == OptionKind::other) {
        return Option{OptionKind::other};
      }
    }

    // A key inside a map or a struct with rfl::ExtraFields. The longest
    // matching prefix wins.
    const std::pair<const std::string, Option>* open = nullptr;
    for (const auto& p : open_prefixes) {
      if (_name.size() > p.first.size() && _name.starts_with(p.first)
          && (!open || p.first.size() > open->first.size())) {
        open = &p;
      }
    }
    if (open) {
      const auto rest = _name.substr(open->first.size());
      const auto sep = rest.find(path_separator);
      if (open->first.empty() && sep != std::string::npos) {
        // A dotted key whose leading segment is a known nested struct or map
        // field (e.g. `--database.nope` beside rfl::ExtraFields) belongs to
        // that field, not to ExtraFields.
        const auto head_prefix = rest.substr(0, sep + 1);
        if (has_prefix(long_options, head_prefix)
            || has_prefix(open_prefixes, head_prefix)) {
          return Option{};
        }
      }
      if (sep == std::string::npos) {
        return open->second;
      }
      const auto nested = open_prefix_options.find(open->first);
      if (nested != open_prefix_options.end()) {
        return nested->second->find_long(rest.substr(sep + 1));
      }
      return open->second.kind == OptionKind::other ? Option{OptionKind::other}
                                                    : Option{};
    }

    return Option{};
  }

  /// Returns the option that the short alias `-_name` stands for.
  Option find_short(const std::string& _name) const {
    const auto it = short_options.find(_name);
    return it == short_options.end() ? Option{} : find_long(it->second);
  }

 private:
  static bool has_prefix(
      const std::map<std::string, Option>& _map, const std::string& _prefix
  ) {
    const auto it = _map.lower_bound(_prefix);
    return it != _map.end() && it->first.starts_with(_prefix);
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

/// What rfl::Short and rfl::Positional say about a field of type T. They are
/// found through rfl::DefaultVal, std::optional, rfl::Description,
/// rfl::Deprecated and rfl::Rename, in any order. `count` is the number of
/// rfl::Short and rfl::Positional wrappers; a field may have at most one.
template <class T>
struct cli_annotations {
  static constexpr bool is_short = false;
  static constexpr bool is_positional = false;
  static constexpr rfl::internal::StringLiteral<1> short_name = "";
  static constexpr int count = 0;
};

template <class T>
using cli_annotations_t = cli_annotations<std::remove_cvref_t<T>>;

template <class T>
struct cli_annotations<DefaultVal<T>> : cli_annotations_t<T> {};

template <class T>
struct cli_annotations<std::optional<T>> : cli_annotations_t<T> {};

template <rfl::internal::StringLiteral _description, class T>
struct cli_annotations<Description<_description, T>> : cli_annotations_t<T> {};

template <rfl::internal::StringLiteral _deprecation_message,
          rfl::internal::StringLiteral _description, class T>
struct cli_annotations<Deprecated<_deprecation_message, _description, T>>
    : cli_annotations_t<T> {};

template <rfl::internal::StringLiteral _name, class T>
struct cli_annotations<Rename<_name, T>> : cli_annotations_t<T> {};

template <class T>
struct cli_annotations<Positional<T>> : cli_annotations_t<T> {
  static constexpr bool is_short = false;
  static constexpr bool is_positional = true;
  static constexpr int count = cli_annotations_t<T>::count + 1;
};

template <rfl::internal::StringLiteral _name, class T>
struct cli_annotations<Short<_name, T>> : cli_annotations_t<T> {
  static constexpr bool is_short = true;
  static constexpr bool is_positional = false;
  static constexpr auto short_name = _name;
  static constexpr int count = cli_annotations_t<T>::count + 1;
};

/// The CLI metadata of the _i-th field of a NamedTuple. InnerType is the
/// declared type of the field, whichever wrappers it consists of.
template <class NamedTupleType, size_t _i>
struct field_info {
  using FieldType = rfl::tuple_element_t<_i, typename NamedTupleType::Fields>;
  using InnerType = typename FieldType::Type;
  using OuterType = std::remove_pointer_t<std::remove_cvref_t<InnerType>>;
  using Annotations = cli_annotations_t<OuterType>;

  static constexpr bool is_positional = Annotations::is_positional;
  static constexpr bool is_short = Annotations::is_short;

  static_assert(Annotations::count <= 1,
                "A field can have only one rfl::Short or rfl::Positional, "
                "wherever it sits among the field's other wrappers.");

  static constexpr std::string_view name() { return FieldType::name(); }
};

/// The short name of the _i-th field, or an empty string if it has none.
template <class NamedTupleType, size_t _i>
constexpr auto get_short_name() {
  using Info = field_info<NamedTupleType, _i>;
  if constexpr (Info::is_short) {
    return Info::Annotations::short_name;
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

template <class... Ts>
struct type_list {
  template <class T>
  static constexpr bool contains = (std::is_same_v<T, Ts> || ...);

  template <class T>
  using add = type_list<Ts..., T>;
};

/// The option for a field, or a key of a map or of rfl::ExtraFields, with
/// values of type T.
template <class T, class ProcessorsType>
Option option_for() {
  using U = unwrapped_option_t<T, ProcessorsType>;
  if constexpr (std::is_same_v<U, bool>) {
    return Option{OptionKind::flag};
  }
  else if constexpr (parsing::is_string_map_v<U> || is_option_struct_v<U>
                     || is_open_option_v<U>) {
    return Option{OptionKind::other};
  }
  else {
    return Option{OptionKind::value};
  }
}

template <class T, class ProcessorsType, class VisitedType>
void collect_options(const std::string& _prefix, Options* _options);

template <class T, class ProcessorsType, class VisitedType>
void collect_value_options(Options* _options);

/// Adds the open prefix `_prefix` of a map or of rfl::ExtraFields with
/// values of type T. If the values are structs or maps, also adds the
/// options inside one of them.
template <class T, class ProcessorsType, class VisitedType>
void add_open_prefix(const std::string& _prefix, Options* _options) {
  using U = unwrapped_option_t<T, ProcessorsType>;
  _options->open_prefixes[_prefix] = option_for<T, ProcessorsType>();
  if constexpr (parsing::is_string_map_v<U>
                || (is_option_struct_v<U>
                    && !VisitedType::template contains<U>)) {
    auto nested = rfl::Box<Options>::make();
    collect_value_options<T, ProcessorsType, VisitedType>(nested.get());
    _options->open_prefix_options.insert_or_assign(_prefix, std::move(nested));
  }
}

/// Adds the options inside one value of type T to `_options`, with paths
/// relative to it: the fields of a struct, or the keys of a map.
template <class T, class ProcessorsType, class VisitedType>
void collect_value_options(Options* _options) {
  using U = unwrapped_option_t<T, ProcessorsType>;
  if constexpr (parsing::is_string_map_v<U>) {
    add_open_prefix<typename U::mapped_type, ProcessorsType, VisitedType>(
        "", _options);
  }
  else if constexpr (is_option_struct_v<U>) {
    collect_options<U, ProcessorsType, VisitedType>("", _options);
  }
  else {
    _options->open_prefixes[""] = Option{OptionKind::other};
  }
}

template <class T, class ProcessorsType, class VisitedType>
void add_option(const std::string& _path, Options* _options) {
  using U = unwrapped_option_t<T, ProcessorsType>;
  if constexpr (std::is_same_v<U, bool>) {
    _options->long_options[_path] = Option{OptionKind::flag};
  }
  else if constexpr (is_open_option_v<U>) {
    _options->long_options[_path] = Option{OptionKind::other};
  }
  else if constexpr (parsing::is_string_map_v<U>) {
    add_open_prefix<typename U::mapped_type, ProcessorsType, VisitedType>(
        _path + path_separator, _options);
  }
  else if constexpr (is_option_struct_v<U>) {
    if constexpr (VisitedType::template contains<U>) {
      // A recursive type: stop here and let the reader handle deeper keys.
      _options->open_prefixes[_path + path_separator] =
          Option{OptionKind::other};
    }
    else {
      collect_options<U, ProcessorsType, VisitedType>(
          _path + path_separator, _options);
    }
  }
  else {
    _options->long_options[_path] = Option{OptionKind::value};
  }
}

template <class FieldType, class ProcessorsType, class VisitedType>
void add_option_for_field(const std::string& _prefix, Options* _options) {
  using T = std::remove_cvref_t<typename FieldType::Type>;
  if constexpr (rfl::internal::is_extra_fields_v<T>) {
    add_open_prefix<typename T::mapped_type, ProcessorsType, VisitedType>(
        _prefix, _options);
  }
  else {
    add_option<T, ProcessorsType, VisitedType>(
        _prefix + std::string(FieldType::name()), _options);
  }
}

template <class T, class ProcessorsType, class VisitedType>
void collect_options(const std::string& _prefix, Options* _options) {
  using NT = option_fields_t<T, ProcessorsType>;
  using NextVisitedType = typename VisitedType::template add<T>;
  [&]<size_t... _is>(std::index_sequence<_is...>) {
    (add_option_for_field<
         rfl::tuple_element_t<_is, typename NT::Fields>, ProcessorsType,
         NextVisitedType>(_prefix, _options),
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

template <class FieldType, class ProcessorsType>
void add_top_level_option(Options* _options) {
  using T = std::remove_cvref_t<typename FieldType::Type>;
  if constexpr (!rfl::internal::is_extra_fields_v<T>) {
    _options->long_options[std::string(FieldType::name())] =
        option_for<T, ProcessorsType>();
  }
}

/// Adds the options of the top-level fields of T, which is all that short
/// options need.
template <class T, class ProcessorsType>
void collect_top_level_options(Options* _options) {
  using NT = option_fields_t<T, ProcessorsType>;
  [&]<size_t... _is>(std::index_sequence<_is...>) {
    (add_top_level_option<rfl::tuple_element_t<_is, typename NT::Fields>,
                          ProcessorsType>(_options),
     ...);
  }(std::make_index_sequence<NT::size()>());
}

/// Derives the options that T accepts from its fields: the long options,
/// with whether each is a flag or takes a value, and the rfl::Short aliases
/// and rfl::Positional fields of the top-level struct.
/// @tparam T The target struct type
/// @tparam ProcessorsType The processors applied to the struct
/// @tparam _all_paths Whether to list every long option through nested
/// structs, maps and rfl::ExtraFields, rather than only the top-level fields.
/// Only rfl::NoExtraFields needs them all, to reject unknown options.
/// @return The options, with reject_unknown set if ProcessorsType contains
/// rfl::NoExtraFields
template <class T, class ProcessorsType,
          bool _all_paths = rfl::internal::no_extra_fields_v<ProcessorsType>>
Options make_options() {
  Options options;
  options.group_short_options = true;
  options.reject_unknown = rfl::internal::no_extra_fields_v<ProcessorsType>;
  using U = unwrapped_option_t<T, ProcessorsType>;
  if constexpr (_all_paths) {
    collect_value_options<T, ProcessorsType, type_list<>>(&options);
  }
  else if constexpr (is_option_struct_v<U>) {
    collect_top_level_options<U, ProcessorsType>(&options);
  }
  if constexpr (is_option_struct_v<U>) {
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
