---
breadcrumbs: true
---

# rfl::cli — Command-Line Argument Parser

Parse `argc`/`argv` into any reflectable struct via `rfl::cli::read<T>(argc, argv)`.

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

## Long options

A long option takes its value either after `=` or as the next token:

```
./app --host-name=localhost --port=8080
./app --host-name localhost --port 8080 --tags a,b,c --rate -0.5
```

This works for every field that takes a value: nested fields
(`--database.host db.local`), vectors, enums, numbers, optionals, map
entries (`--labels.env prod`), fields inlined by `rfl::Flatten`, and fields
wrapped in `rfl::Short`, `rfl::Positional`, `rfl::Rename`,
`rfl::Description`, `rfl::Validator` or `rfl::DefaultVal`.

The next token is the value unless it looks like an option (`-` followed by
a letter), so negative numbers such as `--offset -5` work. If the value is
missing, parsing fails with `Missing value for option '--sql'.` Use `=` to
pass an empty value or one that starts with `-`: `--sql=`, `--pattern=-x`.

Boolean fields (`bool`, `std::optional<bool>` and wrapped bools) are flags
and do not consume the next token: `--verbose input.txt` sets `verbose` and
passes `input.txt` as a positional argument. Like short bool flags, a long
flag takes the next token only if it is `true`, `false`, `1` or `0`, so
`--verbose false` sets `verbose` to `false`. To pass such a token as a
positional argument, write `--verbose -- 1` or `--verbose=true 1`.

## Unknown options

By default, long options that match no field are ignored, and they do not
consume the next token. Pass `rfl::NoExtraFields` to reject them:

```cpp
const auto result = rfl::cli::read<Config, rfl::NoExtraFields>(argc, argv);
// ./app --nope 1   ->  error: Unknown option '--nope'.
```

Unknown short options are always an error.

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

Short bool flags do not consume the next token as a value — `-v somefile`
treats `somefile` as a positional argument, not as the value of `-v`.
To explicitly set a bool short flag, use `=` syntax: `-v=true`, `-v=false`.

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
   and `positional` buckets (`ParsedArgs` struct). It is given a
   `LongOptions` table derived from the target struct
   (`make_long_options<T, ProcessorsType>()`), which says which long options
   are flags and which take a value, so that `--key value` works.
2. **`resolve_args`** — uses compile-time metadata from the target struct to
   map short aliases to long names, reclaim values from bool short flags,
   and merge positional arguments. Produces a flat `map<string, string>`.
3. **`Reader`** — implements reflect-cpp's `IsReader` concept by presenting
   virtual tree nodes over the flat map. Each node is a `{map*, path}` pair —
   no data copying, just prefix-based lookup via `lower_bound`.

## Files

- `include/rfl/cli/read.hpp` — public API
- `include/rfl/cli/Reader.hpp` — Reader + `parse_value` overloads
- `include/rfl/cli/Parser.hpp` — Parser type alias
- `include/rfl/cli/parse_argv.hpp` — `argv` -> `ParsedArgs`
- `include/rfl/cli/long_options.hpp` — `LongOptions`: which long options take a value
- `include/rfl/cli/resolve_args.hpp` — `ParsedArgs` -> `map<string, string>`
- `include/rfl/cli.hpp` — aggregator header
- `include/rfl/SnakeCaseToKebabCase.hpp` — processor
- `include/rfl/Positional.hpp` — `Positional<T>` wrapper
- `include/rfl/Short.hpp` — `Short<"x", T>` wrapper
