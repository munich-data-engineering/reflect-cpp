#ifndef RFL_CLI_PARSE_ARGV_HPP_
#define RFL_CLI_PARSE_ARGV_HPP_

#include <cctype>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "../Result.hpp"
#include "../internal/strings/utf8_conversions.hpp"
#include "resolve_args.hpp"

namespace rfl::cli {

/// Returns true if the token looks like a CLI option (starts with '-'
/// followed by a letter), as opposed to a negative number like "-42".
/// This helps distinguish option flags from negative numeric values.
/// @param _token The command-line token to check
/// @return true if it looks like an option, false otherwise
inline bool looks_like_option(std::string_view _token) noexcept {
  return _token.size() >= 2
         && _token[0] == '-'
         && !std::isdigit(static_cast<unsigned char>(_token[1]))
         && _token[1] != '.';
}

namespace internal {

/// Parses command-line arguments as parse_argv does. A token that looks like
/// a negative number (-5, -.5) is a positional argument unless
/// `_negative_numbers_are_options` is true, as it is when the target struct
/// has a short option whose name looks like a number (rfl::Short<"1", ...>).
/// Then such tokens are short options, as in argparse.
inline rfl::Result<ParsedArgs> parse_args(int argc, char* argv[],
                                          bool _negative_numbers_are_options) {
  if (argc < 0 || (argc > 0 && !argv)) {
    return error("Invalid argc/argv.");
  }
  ParsedArgs result;
  if (argc <= 1) {
    return result;
  }

  const auto add_short = [&](std::string _key, std::string _val,
                             std::string_view _raw) -> rfl::Result<bool> {
    result.short_order.emplace_back(_key);
    if (!result.short_args.emplace(std::move(_key), std::move(_val)).second) {
      return error("Duplicate short argument: " + std::string(_raw));
    }
    return true;
  };

  const auto args = std::span(argv + 1, argc - 1);
  bool force_positional = false;
  for (size_t i = 0; i < args.size(); ++i) {
    const std::string_view arg_raw(args[i]);

    if (force_positional) {
      result.positional.emplace_back(arg_raw);
      continue;
    }

    // "--" separator: everything after is positional.
    if (arg_raw == "--") {
      force_positional = true;
      continue;
    }

    // Long argument: --key=value or --flag.
    if (arg_raw.starts_with("--")) {
      const auto arg = arg_raw.substr(2);
      const auto eq = arg.find('=');
      auto key = std::string(
          eq == std::string_view::npos ? arg : arg.substr(0, eq));
      auto val = std::string(
          eq == std::string_view::npos ? "" : arg.substr(eq + 1));
      if (key.empty()) {
        return error("Empty key in argument: " + std::string(arg_raw));
      }
      if (!result.named.emplace(std::move(key), std::move(val)).second) {
        return error("Duplicate argument: " + std::string(arg_raw));
      }
      continue;
    }

    // Short argument: -x value, -x=value, or -x (bool).
    // A negative number such as -5 or -.5 is not an option; it falls through
    // to positional, unless the struct has a short option such as -1.
    if (looks_like_option(arg_raw)
        || (_negative_numbers_are_options && arg_raw.size() >= 2
            && arg_raw[0] == '-')) {
      const auto arg = arg_raw.substr(1);
      const auto eq = arg.find('=');

      // -x=value
      if (eq != std::string_view::npos) {
        const auto r = add_short(
            std::string(arg.substr(0, eq)),
            std::string(arg.substr(eq + 1)), arg_raw);
        if (!r) { return error(r.error().what()); }
        continue;
      }

      // -x value or -x (bool flag).
      // Peek at next token: consume as value if it doesn't look like an option.
      auto val = std::string();
      if (i + 1 < args.size() && !looks_like_option(args[i + 1])) {
        val = std::string(args[++i]);
      }
      const auto r = add_short(std::string(arg), std::move(val), arg_raw);
      if (!r) { return error(r.error().what()); }
      continue;
    }

    // Bare argument → positional.
    result.positional.emplace_back(arg_raw);
  }
  return result;
}

}  // namespace internal

/// Parses command-line arguments into categorized buckets.
/// Handles three types of arguments:
/// - Long options: --key=value or --flag (stored in `named`)
/// - Short options: -x value, -x=value, or -x (stored in `short_args`)
/// - Positional arguments: bare arguments not prefixed with dashes (stored in `positional`)
/// The special argument "--" forces all subsequent arguments to be treated as positional.
/// Without the target struct, a token such as -5 or -.5 is a short option.
/// rfl::cli::read, which knows the struct, reads it as a negative number
/// unless the struct has a short option like rfl::Short<"1", ...>.
/// @param argc Number of command-line arguments
/// @param argv Array of command-line argument strings
/// @return A Result containing ParsedArgs with categorized arguments or an error
/// --key=value / --flag → named
/// -x value / -x=value / -x → short_args
/// bare arguments → positional
/// -- → everything after goes to positional
inline rfl::Result<ParsedArgs> parse_argv(int argc, char* argv[]) {
  return internal::parse_args(argc, argv, true);
}

/// Parses wide-character command-line arguments into categorized buckets.
/// Converts each wide string to UTF-8 and delegates to the narrow parse_argv.
/// @param argc Number of wide-character command-line arguments
/// @param argv Array of wide-character command-line argument strings
/// @return A Result containing ParsedArgs with UTF-8 encoded arguments
inline rfl::Result<ParsedArgs> parse_argv(int argc, wchar_t* argv[]) {
  if (argc < 0 || (argc > 0 && !argv)) {
    return error("Invalid argc/argv.");
  }
  if (argc <= 1) {
    return ParsedArgs{};
  }

  std::vector<std::string> narrow_argv;
  narrow_argv.reserve(argc);
  std::vector<char*> narrow_argv_ptrs;
  narrow_argv_ptrs.reserve(argc);

  for (int i = 0; i < argc; ++i) {
    const auto str = rfl::internal::strings::wstring_to_utf8(argv[i]);
    if (!str) {
      return error(
          "Could not convert argument " + std::to_string(i)
          + " from wide to UTF-8.");
    }
    narrow_argv.emplace_back(std::move(*str));
    narrow_argv_ptrs.push_back(narrow_argv.back().data());
  }

  return parse_argv(argc, narrow_argv_ptrs.data());
}

}  // namespace rfl::cli

#endif
