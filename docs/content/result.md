# `rfl::Result`

In this documentation, we have, on several occasions, read a JSON string like this:

```cpp
const auto homer = rfl::json::read<Person>(json_string).value();
```

However, so far we have not elaborated on why `.value()` is necessary. Why can't we just do this?

```cpp
const auto homer = rfl::json::read<Person>(json_string);
```

The reason is that parsing a JSON string is something that might fail. We do not know much about
the string that put into there. It might not be a valid JSON. And even if it is a valid JSON, then it
might not conform to the assumptions that we have made about it and laid out in the type system
in the form of various structs and containers.

`rfl::json::read` handles this by returning an `rfl::Result` type. `rfl::Result` is a way of error
handling without exceptions. This is needed for two reasons:

1) Exceptions are controversial, because they add another, hidden control
path to your program that it hard to follow or predict. For this reasons, and others, the Google
C++ style guide [disallows exceptions altogether](https://google.github.io/styleguide/cppguide.html#Exceptions).
Modern programming languages like Go and Rust don't even support them in the first place. The C++
standards committee has recognized this and introduced [std::expected](https://en.cppreference.com/w/cpp/utility/expected)
in C++-23. However, reflect-cpp is a library for C++-20 and until that changes, we need to implement our own result type.

2) In some cases, you want to signal to the parser that is fine if some things go wrong. `rfl::Result` is the way to do that.

If you do not care about these objections to exceptions, you can just call `rfl::json::read<...>(json_string).value()`,
which throws an exception, if the result type contains an error and be done with it.

**Note**: If you want to use `std::expected` instead of our own Result type, you can pass `-DREFLECTCPP_USE_STD_EXPECTED` during compilation.
This requires C++-23 support and may not be supported on all C++ compilers.

## What is `rfl::Result`?

`rfl::Result` is similar to `std::optional` it that it contains an object that may or may not be there. However,
unlike `std::optional`, it does not simply contain `std::nullopt` if the operation has not been successful. Instead,
it contains an `rfl::Error` with a clear error message.

In laying out our structs an containers, about the the type system, we are making *requirements* about the
JSON input we are expecting from the outside world. `rfl::json::read<T>(...)` checks whether these requirements are met.
If the requirements are met, `rfl::json::read<T>` returns T wrapped inside `rfl::Result`. If they are not met, it returns
`rfl::Error` containing an error message explaining what went wrong.

## Using `rfl::Result` for parsing

Suppose you have a vector with 1000 user-supplied configurations:

```cpp
const auto configs = rfl::json::read<std::vector<Config>>(json_string);
```

Chances are that the user of your software has made as mistake in at least some of these configurations. If you set it up as shown above, `rfl::json::read` will fail if one or more `Config` items contain an error. But sometimes you don't want that. Sometimes you want your software to proceed with the remaining 999 configurations and gently point out to the user that some of the configurations were faulty.

The solution is to this instead:

```cpp
const auto configs = rfl::json::read<std::vector<rfl::Result<Config>>>(json_string);
```

This means `rfl::json::read` is going to cut the end-users of your software some slack. But it also means that you, as the programmer, have to handle the fact that some of the configs might contain errors.

Below, we will show you how to do that.

## Monadic operations

From the point of view of category theory, `rfl::Result` is both a functor and a monad (actually, all monads are functors).

This is reflected in its member functions `.transform(...)` and `.and_then(...)`.

`.transform(...)` requires a function `F` of the type `T -> U`. If the `rfl::Result<T>` contains a type `T`,
then `r.transform(f)` *will* return an `rfl::Result<U>` containing a type `U`, otherwise it will return an `rfl::Result<U>`
containing an error.

`.and_then(...)` requires a function `F` of the type `T -> rfl::Result<U>`. If the `rfl::Result<T>` contains a type `T`,
then `r.and_then(f)` *might* return an `rfl::Result<U>` containing a type `U`, depending on whether `f` failed.
If `r` already contained an error, then `r.and_then(f)` will surely return an `rfl::Result<U>` containing an error.

Throughout reflect-cpp's code, these functions are widely used by the parser. For instance, consider how we are reading
`std::unique_ptr<T>` (note that this is somewhat simplified):

```cpp
const auto to_ptr = [](auto&& _t) {
    return std::make_unique<T>(std::move(_t));
};

return Parser<T>::read(_r, _var).transform(to_ptr);
```

This code works as follows: First, we read in a type `T`, which is something that might fail. However, we know 100% that
once we have `T` we can surely wrap it inside a pointer, so we can call `.transform(...)`.

On the other hand, consider the implementation of `rfl::json::load`, which loads a JSON from a file:

```cpp
template <class T>
Result<T> load(const std::string& _fname) {
    return rfl::io::load_string(_fname).and_then(read<T>);
}
```

This code works as follows: First, we read in the text file using `rfl::io::load_string`. This an operation that might fail (maybe
the file doesn't exist, maybe it isn't readable, ...). Then we have to to parse its contents using `rfl::json::read<T>`. Again,
this is an operation that might fail (might not be a proper JSON, might not conform to our requirements, ...). Therefore, we
have to call `.and_then(...)`.

## Retrieving values or errors

If you want the underlying value of type T, you can use `.value(...)`. Note that this throws an exception, if `rfl::Result<T>` contains
an error.

If you are 100% sure that the result does not contain error, you can also call the operator `*`. This will lead to undefined behavior,
if you are wrong.

If you want the underlying value of type T or some kind of default value, you can call `.value_or(your_default_value)`.

If you want to retrieve the error, you can call `.error()`, which will return a value of type `rfl::Error`.

If you want to check whether the result contains an error, you can just use the bool operator:

```cpp
if (my_result) {
  // my_result cannot contain an error,
  // so it is okay to call operator *.
  auto v = *my_result;
} else {
  // my_result must contain an error.
  auto err = my_result.error();
}
```

## Locating errors

`.what()` returns a message meant to be read by humans. When an error summarizes several failures, for example because more than one field is invalid, the message lists all of them:

```
Found 2 errors:
1) Failed to parse field 'inputFile': Size validation failed: Value expected to be greater than or equal to 5, but got 4.
2) Field named 'outputFile' not found.
```

If your program needs to know where each failure occurred, for instance to show each message next to the right input in a form, use `.details()` instead. It returns a `std::vector<rfl::ErrorDetail>` with one entry per failure:

```cpp
struct ErrorDetail {
  using PathSegment = std::variant<std::string, size_t>;

  // Field names and array indices, starting from the root.
  std::vector<PathSegment> path;

  // The message, without the "Failed to parse field '...': " prefixes.
  std::string message;
};
```

Field names are spelled as they appear in the input, that is, after processors such as `rfl::SnakeCaseToCamelCase` or `rfl::SnakeCaseToKebabCase` (which `rfl::cli::read` uses) have been applied. This means the paths line up with the JSON schema generated by `rfl::json::to_schema`.

```cpp
struct FileSpec {
  rfl::Validator<std::string, rfl::Size<rfl::Minimum<5>>> input_file;
  std::string output_file;
};

struct Request {
  std::vector<FileSpec> files;
};

const auto result = rfl::json::read<Request, rfl::SnakeCaseToCamelCase>(
    R"({"files":[{"inputFile":"a.txt","outputFile":"a.out"},{"inputFile":"b"}]})");

for (const auto& detail : result.error().details()) {
  // First iteration:  path = {"files", 1, "inputFile"},
  //                   message = "Size validation failed: ..."
  // Second iteration: path = {"files", 1, "outputFile"},
  //                   message = "Field named 'outputFile' not found."
}
```

An error that does not belong to any particular field, such as a syntax error in the document, is reported as a single detail with an empty path. Untagged variants are also reported as a single detail at the location of the variant, because reflect-cpp cannot know which alternative the input was meant to be. For `rfl::TaggedUnion`, the tag determines the alternative, so the paths extend into the fields of that alternative.

## `rfl::error`

`rfl::error("error message")` is a simple utility function for `rfl::Unexpected<Error>(Error("error message"))`.

## `.or_else(...)`, `.transform_error(...)`

`r.or_else(f)` expects a function `f` for type `Error -> rfl::Result<T>`. It returns `r` if `r` did not contain an error and the results of `f` otherwise.

`r.transform_error(f)` expects a function `f` for type `Error -> Error`.

This is often used to produce better error messages:

```cpp
const auto embellish_error = [&](const Error& _e) -> rfl::Result<T> {
    return rfl::error("Failed to parse field '" + key + "': " + _e.what());
};
return Parser<T>::read(_r, &_var).transform_error(embellish_error);
```

## See also

- [`std::expected`](../expected) - The C++-23 standard result type, which is supported as a serializable data type.
