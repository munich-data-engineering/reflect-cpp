#ifndef RFL_CLI_READ_HPP_
#define RFL_CLI_READ_HPP_

#include <initializer_list>
#include <string_view>
#include <utility>

#include "../Processors.hpp"
#include "../SnakeCaseToKebabCase.hpp"
#include "Parser.hpp"
#include "Reader.hpp"
#include "parse_argv.hpp"
#include "resolve_args.hpp"

namespace rfl::cli {

namespace internal {

/// True if T has a short option whose name looks like a number, such as
/// rfl::Short<"1", ...>. Then, as in argparse, tokens like -5 and -.5 are
/// short options rather than negative numbers.
template <class T, class... Ps>
constexpr bool negative_numbers_are_options() {
  using ProcessorsType = Processors<SnakeCaseToKebabCase, Ps...>;
  using NT = rfl::internal::processed_t<T, ProcessorsType>;
  return has_numeric_short_name<NT>(std::make_index_sequence<NT::size()>{});
}

/// Turns categorized arguments into a struct.
template <class T, class... Ps>
rfl::Result<T> read_parsed(ParsedArgs _parsed) {
  using ProcessorsType = Processors<SnakeCaseToKebabCase, Ps...>;
  return resolve_args<T, ProcessorsType>(std::move(_parsed))
      .and_then([](auto _args) -> rfl::Result<T> {
        const auto r = Reader();
        const auto var = CliVarType{&_args, "", std::nullopt};
        return Parser<T, ProcessorsType>::read(r, var);
      });
}

}  // namespace internal

/// Parses a sequence of command-line arguments into a struct using reflection.
/// Field names are automatically converted from snake_case to kebab-case for CLI arguments.
/// For example, a struct field named `host_name` will match the CLI argument `--host-name`.
/// Supports nested objects (e.g., --database.host), arrays (e.g., --ports=8080,8081),
/// positional arguments, and short flags.
///
/// Accepts a range of narrow or wide strings, such as
/// `std::vector<std::string>`, `std::span<const char* const>` or
/// `const wchar_t*[]`, or a braced list of narrow strings such as
/// `{"--port=8080"}`. Wide strings are converted to UTF-8. The number of
/// arguments is taken from the range. The parser keeps a std::string_view to
/// each narrow argument, so a range of narrow strings must be multi-pass and
/// must yield lvalue strings, pointers or string_views, not temporary
/// `std::string`s.
///
/// Every element is parsed: unlike `read(argc, argv)`, the first one is NOT
/// skipped as the program name. To pass main()'s arguments, drop argv[0]
/// yourself: `std::span(argv, argc)` would parse the program name as an
/// argument.
/// @tparam T The struct type to parse into
/// @tparam Ps Optional processors to apply during parsing
/// @param _args The command-line arguments to parse
/// @return A Result containing the parsed struct or an error
template <class T, class... Ps,
          class R = std::initializer_list<std::string_view>>
  requires internal::StringRange<R> || internal::WideStringRange<R>
rfl::Result<T> read(R&& _args) {
  return internal::parse_args(std::forward<R>(_args),
                              internal::negative_numbers_are_options<T, Ps...>())
      .and_then(internal::read_parsed<T, Ps...>);
}

/// Parses command-line arguments into a struct using reflection.
/// argv[0] is skipped as the program name. See the sequence overload above
/// for the supported syntax.
/// @tparam T The struct type to parse into
/// @tparam Ps Optional processors to apply during parsing
/// @param argc Number of command-line arguments
/// @param argv Array of command-line argument strings
/// @return A Result containing the parsed struct or an error
template <class T, class... Ps>
rfl::Result<T> read(int argc, const char* const argv[]) {
  return internal::parse_args(argc, argv,
                              internal::negative_numbers_are_options<T, Ps...>())
      .and_then(internal::read_parsed<T, Ps...>);
}

template <class T, class... Ps>
rfl::Result<T> read(int argc, char* argv[]) {
  return read<T, Ps...>(argc, static_cast<const char* const*>(argv));
}

/// Parses wide-character command-line arguments into a struct using reflection.
/// argv[0] is skipped as the program name. Wide strings are converted to
/// UTF-8 before parsing.
/// @tparam T The struct type to parse into
/// @tparam Ps Optional processors to apply during parsing
/// @param argc Number of wide-character command-line arguments
/// @param argv Array of wide-character argument strings
/// @return A Result containing the parsed struct or an error
template <class T, class... Ps>
rfl::Result<T> read(int argc, const wchar_t* const argv[]) {
  return internal::parse_args(argc, argv,
                              internal::negative_numbers_are_options<T, Ps...>())
      .and_then(internal::read_parsed<T, Ps...>);
}

template <class T, class... Ps>
rfl::Result<T> read(int argc, wchar_t* argv[]) {
  return read<T, Ps...>(argc, static_cast<const wchar_t* const*>(argv));
}

}  // namespace rfl::cli

#endif
