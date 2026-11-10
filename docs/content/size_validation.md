# `rfl::Size`

`rfl::Size` can be used to impose constraints on any container that contains the method `.size()`, which includes `std::string`.

For instance, you can a require a vector to be non-empty as follows:

```cpp
using NonEmptyStringVector =
    rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>>;
```

Or you could require it to contain exactly zero or three elements, using validator composition:

```cpp
using NoneOrThree =
    rfl::Validator<std::vector<std::string>,
                   rfl::Size<rfl::AnyOf<rfl::EqualTo<0>, rfl::EqualTo<3>>>>;
```

Note that for `std::string`, `.size()` returns the number of bytes, not the number of characters.

## `rfl::CodePoints`

JSON Schema's `minLength` and `maxLength` count Unicode code points, whereas `rfl::Size` counts bytes. For UTF-8 strings containing non-ASCII characters, the two disagree: `"héllo"` is 6 bytes, but only 5 code points.

If you want the validation to match the generated JSON schema, use `rfl::CodePoints` instead. It works just like `rfl::Size`, but counts the UTF-8 code points in a string:

```cpp
using Username =
    rfl::Validator<std::string, rfl::CodePoints<rfl::Maximum<16>>>;
```

The generated JSON schema is the same as for `rfl::Size` (`minLength`/`maxLength`).

Keep in mind that `rfl::CodePoints` counts code points, not grapheme clusters (user-perceived characters): an emoji with a skin-tone modifier counts as two. It also does not check that the string is valid UTF-8.
