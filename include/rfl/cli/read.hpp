#ifndef RFL_CLI_READ_HPP_
#define RFL_CLI_READ_HPP_

#include "../Processors.hpp"
#include "../SnakeCaseToKebabCase.hpp"
#include "../internal/strings/utf8_conversions.hpp"
#include "Parser.hpp"
#include "Reader.hpp"
#include "long_options.hpp"
#include "parse_argv.hpp"
#include "resolve_args.hpp"

namespace rfl::cli {

/// Parses command-line arguments into a struct using reflection.
/// Field names are automatically converted from snake_case to kebab-case for CLI arguments.
/// For example, a struct field named `host_name` will match the CLI argument `--host-name`.
/// Supports nested objects (e.g., --database.host), arrays (e.g., --ports=8080,8081),
/// positional arguments, and short flags.
/// @tparam T The struct type to parse into
/// @tparam Ps Optional processors to apply during parsing
/// @param argc Number of command-line arguments
/// @param argv Array of command-line argument strings
/// @return A Result containing the parsed struct or an error
template <class T, class... Ps>
rfl::Result<T> read(int argc, char* argv[]) {
  using ProcessorsType = Processors<SnakeCaseToKebabCase, Ps...>;
  return parse_argv(argc, argv, make_long_options<T, ProcessorsType>())
      .and_then(resolve_args<T, ProcessorsType>)
      .and_then([](auto _args) -> rfl::Result<T> {
        const auto r = Reader();
        const auto var = CliVarType{&_args, "", std::nullopt};
        return Parser<T, ProcessorsType>::read(r, var);
      });
}

/// Parses wide-character command-line arguments into a struct using reflection.
/// Field names are automatically converted from snake_case to kebab-case for CLI arguments.
/// Wide strings are converted to UTF-8 before parsing.
/// @tparam T The struct type to parse into
/// @tparam Ps Optional processors to apply during parsing
/// @param argc Number of wide-character command-line arguments
/// @param argv Array of wide-character argument strings
/// @return A Result containing the parsed struct or an error
template <class T, class... Ps>
rfl::Result<T> read(int argc, wchar_t* argv[]) {
  if (argc < 0 || (argc > 0 && !argv)) {
    return error("Invalid argc/argv.");
  }
  std::vector<std::string> narrow_argv;
  narrow_argv.reserve(argc);
  std::vector<char*> narrow_argv_ptrs;
  narrow_argv_ptrs.reserve(argc);
  for (int i = 0; i < argc; ++i) {
    const auto str = rfl::internal::strings::wstring_to_utf8(argv[i]);
    if (!str) {
      return error("Could not convert argument " + std::to_string(i)
                   + " from wide to UTF-8.");
    }
    narrow_argv.emplace_back(std::move(*str));
    narrow_argv_ptrs.push_back(narrow_argv.back().data());
  }
  return read<T, Ps...>(argc, narrow_argv_ptrs.data());
}

}  // namespace rfl::cli

#endif
