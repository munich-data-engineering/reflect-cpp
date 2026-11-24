#ifndef RFL_CLI_RESOLVE_ARGS_HPP_
#define RFL_CLI_RESOLVE_ARGS_HPP_

#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../Result.hpp"
#include "options.hpp"

namespace rfl::cli {

/// Holds the parsed command-line arguments categorized by type.
/// Used as an intermediate structure before resolving to a flat key-value map.
struct ParsedArgs {
  std::map<std::string, std::string> named;
  std::map<std::string, std::string> short_args;
  std::vector<std::string> positional;
  /// Insertion order of short arg keys (for stable reclaim ordering).
  std::vector<std::string> short_order;
};

namespace internal {

/// Applies what parse_argv could not know without the option table.
/// Without it, every short option takes the next token, so `-v somefile`
/// makes "somefile" the value of -v. If -v is a bool flag (by its type once
/// unwrapped, as in options.hpp), "somefile" is not a valid bool value and
/// is returned to the positional list. We iterate in insertion order
/// (short_order) to preserve argv ordering.
inline void apply_option_kinds(const Options& _options, ParsedArgs* _parsed) {
  std::vector<std::string> reclaimed;
  for (const auto& short_name : _parsed->short_order) {
    const auto option = _options.find_short(short_name);
    auto it = _parsed->short_args.find(short_name);
    if (it == _parsed->short_args.end()) {
      continue;
    }
    auto& value = it->second;
    if (option.kind == OptionKind::flag && !value.empty()
        && !is_bool_literal(value)) {
      reclaimed.push_back(value);
      value.clear();
    }
  }
  // Insert reclaimed values before existing positional args to preserve
  // the original argv order: `-v file1 file2` → positional ["file1", "file2"].
  if (!reclaimed.empty()) {
    _parsed->positional.insert(
        _parsed->positional.begin(),
        std::make_move_iterator(reclaimed.begin()),
        std::make_move_iterator(reclaimed.end()));
  }
}

/// Fails if two fields of the struct have the same rfl::Short name, so
/// every read of such a struct fails.
inline rfl::Result<bool> check_short_names(const Options& _options) {
  if (!_options.duplicate_short_name.empty()) {
    return rfl::error("Duplicate short name '-" + _options.duplicate_short_name
                      + "' in struct definition.");
  }
  return true;
}

/// The error for more positional arguments than positional fields.
inline rfl::Unexpected<Error> too_many_positionals(const size_t _expected,
                                                    const size_t _got) {
  return rfl::error("Too many positional arguments: expected at most "
                    + std::to_string(_expected) + ", got "
                    + std::to_string(_got) + ".");
}

/// Resolves ParsedArgs into a flat key-value map using the options of the
/// target struct.
/// This function:
/// 1. Maps positional arguments to their corresponding field names based on declaration order
/// 2. Expands short arguments (e.g., -p) to long names (e.g., --port) using Short<> annotations
/// 3. Detects conflicts between different ways of specifying the same field
/// The caller checks the short names first (check_short_names).
/// @param _parsed The arguments, parsed by parse_argv with the same options
/// @param _options The options of the target struct (see make_options)
/// @return A Result containing a flat map of field names to values or an
/// error
inline rfl::Result<std::map<std::string, std::string>> resolve_args(
    ParsedArgs _parsed, const Options& _options
) {
  auto& result = _parsed.named;

  // Merge positional args.
  const auto& positional_names = _options.positionals;
  if (_parsed.positional.size() > positional_names.size()) {
    return internal::too_many_positionals(positional_names.size(),
                                          _parsed.positional.size());
  }
  for (size_t i = 0; i < _parsed.positional.size(); ++i) {
    const auto& long_name = positional_names[i];
    if (result.count(long_name)) {
      return rfl::error(
          "Conflict: positional argument and '--" + long_name
          + "' both provided.");
    }
    result.emplace(long_name, std::move(_parsed.positional[i]));
  }

  // Merge short args.
  for (auto& [short_name, value] : _parsed.short_args) {
    const auto it = _options.short_options.find(short_name);
    if (it == _options.short_options.end()) {
      return rfl::error("Unknown short argument: -" + short_name);
    }
    const auto& long_name = it->second;
    if (result.count(long_name)) {
      return rfl::error(
          "Conflict: '-" + short_name + "' and '--" + long_name
          + "' both provided.");
    }
    result.emplace(long_name, std::move(value));
  }

  return std::move(result);
}

}  // namespace internal

/// Resolves ParsedArgs into a flat key-value map using compile-time
/// metadata from the target struct T. Like the overload taking the options
/// of T, but for arguments parsed by parse_argv without them: values that
/// boolean short options took from the next token are returned to the
/// positional arguments.
/// @tparam T The target struct type
/// @tparam ProcessorsType The processors applied to the struct
/// @param _parsed The parsed command-line arguments
/// @return A Result containing a flat map of field names to values or an error
template <class T, class ProcessorsType>
rfl::Result<std::map<std::string, std::string>> resolve_args(
    ParsedArgs _parsed
) {
  const auto options = internal::make_options<T, ProcessorsType>();
  const auto short_names = internal::check_short_names(options);
  if (!short_names) {
    return rfl::error(short_names.error().what());
  }
  internal::apply_option_kinds(options, &_parsed);
  return internal::resolve_args(std::move(_parsed), options);
}

}  // namespace rfl::cli

#endif
