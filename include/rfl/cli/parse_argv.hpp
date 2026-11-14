#ifndef RFL_CLI_PARSE_ARGV_HPP_
#define RFL_CLI_PARSE_ARGV_HPP_

#include <cctype>
#include <concepts>
#include <initializer_list>
#include <map>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
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

/// Parses command-line arguments into categorized buckets.
/// Handles three types of arguments:
/// - Long options: --key=value or --flag (stored in `named`)
/// - Short options: -x value, -x=value, or -x (stored in `short_args`)
/// - Positional arguments: bare arguments not prefixed with dashes (stored in `positional`)
/// The special argument "--" forces all subsequent arguments to be treated as positional.
/// Every element of `_args` is parsed; nothing is skipped as the program name.
/// A token that looks like a negative number (-5, -.5) is a positional
/// argument unless `_negative_numbers_are_options` is true, as it is when the
/// target struct has a short option whose name looks like a number
/// (rfl::Short<"1", ...>). Then such tokens are short options, as in argparse.
/// @param _args The command-line arguments to parse
/// @return A Result containing ParsedArgs with categorized arguments or an error
/// --key=value / --flag → named
/// -x value / -x=value / -x → short_args
/// bare arguments → positional
/// -- → everything after goes to positional
inline rfl::Result<ParsedArgs> parse_string_views(
    std::span<const std::string_view> _args,
    bool _negative_numbers_are_options) {
  ParsedArgs result;

  const auto add_short = [&](std::string _key, std::string _val,
                             std::string_view _raw) -> rfl::Result<bool> {
    result.short_order.emplace_back(_key);
    if (!result.short_args.emplace(std::move(_key), std::move(_val)).second) {
      return error("Duplicate short argument: " + std::string(_raw));
    }
    return true;
  };

  bool force_positional = false;
  for (size_t i = 0; i < _args.size(); ++i) {
    const std::string_view arg_raw(_args[i]);

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
      if (i + 1 < _args.size() && !looks_like_option(_args[i + 1])) {
        val = std::string(_args[++i]);
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

/// An element type that a `std::string_view` can point into after the
/// iterator has moved on: an lvalue reference to a string, a pointer, or a
/// `std::string_view`. A temporary `std::string` is not one, because the
/// view would dangle.
template <class Ref>
concept StableStringReference =
    std::convertible_to<Ref, std::string_view> &&
    (std::is_lvalue_reference_v<Ref> || std::is_pointer_v<Ref> ||
     std::same_as<std::remove_cv_t<Ref>, std::string_view>);

/// A range of narrow strings, such as `std::vector<std::string>`,
/// `std::span<const char* const>` or `const char*[]`. The parser keeps a
/// `std::string_view` to every element, so the range must be multi-pass
/// (`std::views::istream` is rejected) and must not yield temporary
/// `std::string`s.
template <class R>
concept StringRange =
    std::ranges::forward_range<R> &&
    StableStringReference<std::ranges::range_reference_t<R>>;

/// A range of wide strings, such as `std::vector<std::wstring>` or
/// `const wchar_t*[]`. Every element is converted to UTF-8 before the next
/// one is read, so single-pass ranges and temporaries are fine here.
template <class R>
concept WideStringRange =
    std::ranges::input_range<R> &&
    std::convertible_to<std::ranges::range_reference_t<R>, std::wstring_view>;

template <class S>
bool is_null(const S& _str) noexcept {
  if constexpr (std::is_pointer_v<S>) {
    return _str == nullptr;
  } else {
    return false;
  }
}

/// Parses every element of `_args`, rejecting null pointers.
/// `_first_index` is the index of the first element in error messages.
template <StringRange R>
rfl::Result<ParsedArgs> parse_strings(R&& _args, size_t _first_index,
                                      bool _negative_numbers_are_options) {
  std::vector<std::string_view> views;
  auto i = _first_index;
  for (auto&& arg : _args) {
    if (is_null(arg)) {
      return error("Argument " + std::to_string(i) + " is a null pointer.");
    }
    const std::string_view view = arg;
    views.push_back(view);
    ++i;
  }
  return parse_string_views(views, _negative_numbers_are_options);
}

/// Converts every element of `_args` to UTF-8 and parses it.
/// `_first_index` is the index of the first element in error messages.
template <WideStringRange R>
rfl::Result<ParsedArgs> parse_wide_strings(
    R&& _args, size_t _first_index, bool _negative_numbers_are_options) {
  std::vector<std::string> narrow;
  auto i = _first_index;
  for (auto&& arg : _args) {
    if (is_null(arg)) {
      return error("Argument " + std::to_string(i) + " is a null pointer.");
    }
    const std::wstring_view view = arg;
    auto str = rfl::internal::strings::wstring_to_utf8(std::wstring(view));
    if (!str) {
      return error("Could not convert argument " + std::to_string(i) +
                   " from wide to UTF-8.");
    }
    narrow.emplace_back(std::move(*str));
    ++i;
  }
  return parse_strings(narrow, _first_index, _negative_numbers_are_options);
}

/// Validates argc/argv and returns the arguments after argv[0], the program
/// name.
template <class CharT>
rfl::Result<std::span<const CharT* const>> skip_program_name(
    int argc, const CharT* const argv[]) {
  if (argc < 0 || (argc > 0 && !argv)) {
    return error("Invalid argc/argv.");
  }
  if (argc <= 1) {
    return std::span<const CharT* const>();
  }
  return std::span<const CharT* const>(argv + 1, static_cast<size_t>(argc - 1));
}

/// Parses every element of `_args`, as parse_argv(_args) does.
/// `_negative_numbers_are_options` is as for parse_string_views.
template <class R>
  requires StringRange<R> || WideStringRange<R>
rfl::Result<ParsedArgs> parse_args(R&& _args,
                                   bool _negative_numbers_are_options) {
  if constexpr (StringRange<R>) {
    return parse_strings(std::forward<R>(_args), 0,
                         _negative_numbers_are_options);
  } else {
    return parse_wide_strings(std::forward<R>(_args), 0,
                              _negative_numbers_are_options);
  }
}

/// Parses argc/argv, skipping argv[0], as parse_argv(argc, argv) does.
/// `_negative_numbers_are_options` is as for parse_string_views.
template <class CharT>
rfl::Result<ParsedArgs> parse_args(int argc, const CharT* const argv[],
                                   bool _negative_numbers_are_options) {
  return skip_program_name(argc, argv).and_then([&](auto _args) {
    if constexpr (std::is_same_v<CharT, char>) {
      return parse_strings(_args, 1, _negative_numbers_are_options);
    } else {
      return parse_wide_strings(_args, 1, _negative_numbers_are_options);
    }
  });
}

}  // namespace internal

/// Parses a sequence of command-line arguments into categorized buckets.
/// Accepts a range of narrow or wide strings (wide strings are converted
/// to UTF-8) or a braced list of narrow strings. A range of narrow strings
/// must be multi-pass and must not yield temporary `std::string`s, as for
/// rfl::cli::read. Every element is parsed: unlike the argc/argv overloads,
/// the first one is NOT skipped as the program name.
/// Without the target struct, a token such as -5 or -.5 is a short option.
/// rfl::cli::read, which knows the struct, reads it as a negative number
/// unless the struct has a short option like rfl::Short<"1", ...>.
/// @param _args The command-line arguments to parse
/// @return A Result containing ParsedArgs with categorized arguments or an error
template <class R = std::initializer_list<std::string_view>>
  requires internal::StringRange<R> || internal::WideStringRange<R>
rfl::Result<ParsedArgs> parse_argv(R&& _args) {
  return internal::parse_args(std::forward<R>(_args), true);
}

/// Parses command-line arguments into categorized buckets, skipping argv[0]
/// (the program name).
/// @param argc Number of command-line arguments
/// @param argv Array of command-line argument strings
/// @return A Result containing ParsedArgs with categorized arguments or an error
inline rfl::Result<ParsedArgs> parse_argv(int argc, const char* const argv[]) {
  return internal::parse_args(argc, argv, true);
}

inline rfl::Result<ParsedArgs> parse_argv(int argc, char* argv[]) {
  return parse_argv(argc, static_cast<const char* const*>(argv));
}

/// Parses wide-character command-line arguments into categorized buckets,
/// skipping argv[0] (the program name). Converts each wide string to UTF-8.
/// @param argc Number of wide-character command-line arguments
/// @param argv Array of wide-character command-line argument strings
/// @return A Result containing ParsedArgs with UTF-8 encoded arguments
inline rfl::Result<ParsedArgs> parse_argv(int argc,
                                          const wchar_t* const argv[]) {
  return internal::parse_args(argc, argv, true);
}

inline rfl::Result<ParsedArgs> parse_argv(int argc, wchar_t* argv[]) {
  return parse_argv(argc, static_cast<const wchar_t* const*>(argv));
}

}  // namespace rfl::cli

#endif
