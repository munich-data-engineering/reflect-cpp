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
#include "long_options.hpp"
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

/// Returns true if the token is one of the values that a bool flag given
/// without '=' takes from the next token: "true", "false", "1" or "0".
/// @param _token The command-line token to check
/// @return true if it is a boolean literal, false otherwise
inline bool is_bool_literal(std::string_view _token) noexcept {
  return _token == "true" || _token == "false" || _token == "1"
         || _token == "0";
}

/// Parses command-line arguments into categorized buckets.
/// Handles three types of arguments:
/// - Long options: --key=value, --key value or --flag (stored in `named`)
/// - Short options: -x value, -x=value, or -x (stored in `short_args`)  
/// - Positional arguments: bare arguments not prefixed with dashes (stored in `positional`)
/// The special argument "--" forces all subsequent arguments to be treated as positional.
/// `_long_options` says which long options take a value. A long option that
/// takes a value consumes the next token when it is given without '='. A
/// boolean one consumes it only if it is "true", "false", "1" or "0". Others
/// never consume it.
/// @param argc Number of command-line arguments
/// @param argv Array of command-line argument strings
/// @param _long_options The long options of the target struct
/// @return A Result containing ParsedArgs with categorized arguments or an error
/// --key=value / --key value / --flag → named
/// -x value / -x=value / -x → short_args
/// bare arguments → positional
/// -- → everything after goes to positional
inline rfl::Result<ParsedArgs> parse_argv(
    int argc, char* argv[], const LongOptions& _long_options
) {
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

    // Long argument: --key=value, --key value or --flag.
    if (arg_raw.starts_with("--")) {
      const auto arg = arg_raw.substr(2);
      const auto eq = arg.find('=');
      auto key = std::string(
          eq == std::string_view::npos ? arg : arg.substr(0, eq));
      if (key.empty()) {
        return error("Empty key in argument: " + std::string(arg_raw));
      }
      const auto kind = _long_options.kind_of(key);
      if (kind == LongOptionKind::unknown && _long_options.reject_unknown) {
        return error("Unknown option '--" + key + "'.");
      }
      auto val = std::string();
      if (eq != std::string_view::npos) {
        val = std::string(arg.substr(eq + 1));
      }
      else if (kind == LongOptionKind::value) {
        // --key value
        if (i + 1 >= args.size() || looks_like_option(args[i + 1])) {
          return error("Missing value for option '--" + key + "'.");
        }
        val = std::string(args[++i]);
      }
      else if (kind == LongOptionKind::flag) {
        // --flag true / --flag false, like -v true / -v false. Any other
        // token stays where it is, so `--verbose input.txt` leaves
        // "input.txt" positional.
        if (i + 1 < args.size() && is_bool_literal(args[i + 1])) {
          val = std::string(args[++i]);
        }
      }
      if (!result.named.emplace(std::move(key), std::move(val)).second) {
        return error("Duplicate argument: " + std::string(arg_raw));
      }
      continue;
    }

    // Short argument: -x value, -x=value, or -x (bool).
    if (arg_raw.size() >= 2 && arg_raw[0] == '-') {
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

/// Parses command-line arguments without knowing the target struct.
/// Long options never consume the next token, so values must be passed as
/// --key=value.
/// @param argc Number of command-line arguments
/// @param argv Array of command-line argument strings
/// @return A Result containing ParsedArgs with categorized arguments or an error
inline rfl::Result<ParsedArgs> parse_argv(int argc, char* argv[]) {
  return parse_argv(argc, argv, LongOptions{});
}

/// Parses wide-character command-line arguments into categorized buckets.
/// Converts each wide string to UTF-8 and delegates to the narrow parse_argv.
/// @param argc Number of wide-character command-line arguments
/// @param argv Array of wide-character command-line argument strings
/// @param _long_options The long options of the target struct
/// @return A Result containing ParsedArgs with UTF-8 encoded arguments
inline rfl::Result<ParsedArgs> parse_argv(
    int argc, wchar_t* argv[], const LongOptions& _long_options
) {
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

  return parse_argv(argc, narrow_argv_ptrs.data(), _long_options);
}

/// Parses wide-character command-line arguments without knowing the target
/// struct. See the narrow overload.
/// @param argc Number of wide-character command-line arguments
/// @param argv Array of wide-character command-line argument strings
/// @return A Result containing ParsedArgs with UTF-8 encoded arguments
inline rfl::Result<ParsedArgs> parse_argv(int argc, wchar_t* argv[]) {
  return parse_argv(argc, argv, LongOptions{});
}

}  // namespace rfl::cli

#endif
