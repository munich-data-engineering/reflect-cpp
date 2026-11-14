---
breadcrumbs: true
---

# rfl::cli — Command-Line Argument Parser

Parse `argc`/`argv` into any reflectable struct via `rfl::cli::read<T>(argc, argv)`,
or a sequence of arguments via `rfl::cli::read<T>(args)`.

## Usage

```cpp
#include <rfl/cli.hpp>

struct Config {
  std::string host_name;
  int port;
  bool verbose;
  std::optional<double> rate;
  std::vector<std::string> tags;
};

int main(int argc, char* argv[]) {
  const auto result = rfl::cli::read<Config>(argc, argv);
  // ./app --host-name=localhost --port=8080 --verbose --tags=a,b,c
}
```

Field names undergo automatic `snake_case` -> `kebab-case` conversion:
`host_name` matches `--host-name`.

`argv` may be `char*[]` or `const char*[]`. `argv[0]` is the program name
and is skipped.

## Passing a sequence of arguments

When the arguments do not come straight from `main`, pass them as a range,
such as `std::vector<std::string>`, `std::vector<std::string_view>`,
`std::span<char*>` or `const char* const[]`, or as a braced list of strings.
Ranges of wide strings (`std::vector<std::wstring>`, `const wchar_t*[]`) are
converted to UTF-8. A null pointer in a range or array of pointers is an
error. A braced list holds `std::string_view`s, so a null pointer in it is
undefined behaviour; pass a `std::vector` or an array of pointers instead.

The parser keeps a `std::string_view` to each narrow argument, so a range of
narrow strings must be a `std::forward_range` of lvalue strings, pointers or
`std::string_view`s. `std::views::istream<std::string>`, or a view that
yields temporary `std::string`s, does not compile; copy the arguments into a
`std::vector<std::string>` first.

**Every element is parsed: unlike `read(argc, argv)`, the first one is not
skipped as the program name.** So `std::span(argv, argc)` would read the
program name as an argument (for a struct with a `rfl::Positional` field, as
its first value).

```cpp
// ./app serve --host-name=localhost --port=8080 --verbose --tags=a,b
int main(int argc, char* argv[]) {
  // argv[0] is the program name, argv[1] a subcommand the program handles.
  const auto rest = std::span(argv, argc).subspan(std::min(argc, 2));
  const auto result = rfl::cli::read<Config>(rest);
}

// Arguments from somewhere other than main:
const std::vector<std::string> args = {"--host-name=localhost", "--port=8080",
                                       "--verbose", "--tags=a,b"};
const auto result = rfl::cli::read<Config>(args);
```

`rfl::cli::parse_argv` has the same overloads.

## Positional arguments

Wrap a field with `rfl::Positional<T>` to accept it as a bare (non-flag) argument:

```cpp
struct Config {
  rfl::Positional<std::string> input_file;
  rfl::Positional<std::string> output_file;
  bool verbose;
};

// ./app input.txt output.txt --verbose
```

Positional arguments are matched in declaration order. They can also be
passed as named arguments: `--input-file=input.txt`.

The `--` separator forces all subsequent tokens into positional:

```
./app --verbose -- --not-a-flag.txt
```

A token that starts with `-` and a digit or `.`, such as `-5`, `-1.5` or
`-1e3`, is a negative number, so it is read as a positional argument (or as
the value of a short option: `-n -5`). `-inf` and `-nan` start with a letter
and are read as short options; pass them after `--` or as `--offset=-inf`.

This changes if the struct has a short option whose name looks like a
number, such as `rfl::Short<"1", bool>` for `ls -1`. Then such a token is a
short option, and negative positional arguments need `--`:
`./app -1 -- -5`. Right after a short option, such a token is still read as
that option's value, as any other token is: `-n -1` sets `n` to `-1`.

## Short aliases

Wrap a field with `rfl::Short<"x", T>` to add a single-character alias:

```cpp
struct Config {
  rfl::Short<"p", int> port;
  rfl::Short<"v", bool> verbose;
  std::string host;
};

// ./app -p 8080 -v --host=localhost
// ./app -p=8080 -v --host=localhost
// ./app --port=8080 --verbose --host=localhost   (long names still work)
```

Short bool flags (`bool`, `std::optional<bool>` and wrapped bools) do not
consume the next token as a value — `-v somefile` treats `somefile` as a
positional argument, not as the value of `-v`. Unlike long flags, they take
the next token if it is `true`, `false`, `1` or `0`: `-v false` sets the
flag to `false`. Write `-v -- 1` to pass such a token as a positional
argument. To explicitly set a bool short flag, use `=` syntax: `-v=true`,
`-v=false`.

Any other short option takes the next token as its value unless that token
looks like an option (`-` followed by anything except a digit or `.`):
`-o out.csv`, `-o=out.csv`. If the value is missing, parsing fails with
`Missing value for option '-o'.` Use `=` to pass an empty value or one that
starts with `-`: `-o=`, `-o=-x`.

## Combining Positional and Short

`Positional` and `Short` can be used together in the same struct, but
**cannot be nested** (`Positional<Short<...>>` is a compile-time error):

```cpp
struct Config {
  rfl::Positional<std::string> input_file;
  rfl::Short<"o", std::string> output_dir;
  rfl::Short<"v", bool> verbose;
  int count;
};

// ./app data.csv -o /tmp/out -v --count=10
```

## Supported types

| Type | CLI format | Notes |
|------|-----------|-------|
| `std::string` | `--key=value` | |
| `int`, `long`, ... | `--key=42` | |
| `float`, `double` | `--key=1.5` | |
| `bool` | `--flag` or `--flag=true` | No `=` implies `true` |
| `enum` | `--key=value_name` | Via `rfl::string_to_enum` |
| `std::optional<T>` | omit for `nullopt` | |
| `std::vector<T>` | `--key=a,b,c` | Comma-separated; empty elements skipped |
| Nested struct | `--parent.child=val` | Dot-separated path |
| `rfl::Flatten<T>` | fields inlined | No prefix needed |
| `rfl::Rename<"x", T>` | `--x=val` | Bypasses kebab conversion |
| `rfl::Positional<T>` | bare token | Matched in declaration order |
| `rfl::Short<"x", T>` | `-x value` or `-x=value` | Single-character alias |

## Architecture

Parsing proceeds in three stages:

1. **`parse_argv`** — categorizes raw tokens into `named`, `short_args`,
   and `positional` buckets (`ParsedArgs` struct). `rfl::cli::read` gives
   it an internal option table derived from the target struct, which says
   what each option does with the next token and whether a short option's
   name looks like a number. The public `parse_argv` has no table: it reads
   `-5` as a short option.
2. **`resolve_args`** — uses the same table to map short aliases to long
   names and to merge positional arguments. Produces a flat
   `map<string, string>`. The public `resolve_args<T, ProcessorsType>(parsed)`,
   for arguments parsed without the table, also returns the tokens that bool
   short flags took to the positional arguments.
3. **`Reader`** — implements reflect-cpp's `IsReader` concept by presenting
   virtual tree nodes over the flat map. Each node is a `{map*, path}` pair —
   no data copying, just prefix-based lookup via `lower_bound`.

## Files

- `include/rfl/cli/read.hpp` — public API
- `include/rfl/cli/Reader.hpp` — Reader + `parse_value` overloads
- `include/rfl/cli/Parser.hpp` — Parser type alias
- `include/rfl/cli/parse_argv.hpp` — `argv` -> `ParsedArgs`
- `include/rfl/cli/options.hpp` — the internal option table: the long, short and positional options of a struct
- `include/rfl/cli/resolve_args.hpp` — `ParsedArgs` -> `map<string, string>`
- `include/rfl/cli.hpp` — aggregator header
- `include/rfl/SnakeCaseToKebabCase.hpp` — processor
- `include/rfl/Positional.hpp` — `Positional<T>` wrapper
- `include/rfl/Short.hpp` — `Short<"x", T>` wrapper
