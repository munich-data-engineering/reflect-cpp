# Default values (rfl::DefaultVal)

The `rfl::DefaultVal<T>` wrapper allows a struct field to have a predefined default value when serializing and deserializing. When a field is declared as `rfl::DefaultVal<T>`, the library will accept input that omits that field and will populate it with the provided default (or with a default-constructed T when no explicit default is given).

## Declaration and initialization

You can declare a default-valued field like this:

```cpp
struct Person {
  std::string first_name;                             // required
  rfl::DefaultVal<std::string> last_name = "Simpson"; // has explicit default
  rfl::DefaultVal<std::string> town;                  // default-constructed (empty string)
};
```

DefaultVal behaves like a thin wrapper around the underlying type. You can construct and assign it from the underlying type, from other DefaultVal instances (if convertible), or assign the special token `rfl::Default` to reset it to the default-constructed value (if the type is default-constructible):

```cpp
Person p;
p.last_name = "Smith";            // assign underlying value
p.town = rfl::Default{};            // reset to default (empty string)
std::string s = p.last_name.value();
```

API convenience:

- .get(), .value(), operator()(), operator*(), operator->() — access the underlying value (const and non-const overloads).
- set(...) — assign underlying value.

## JSON behaviour

When writing JSON, fields that are DefaultVal are written like normal fields using their current underlying value. When reading JSON, omitted DefaultVal fields are filled with the default value (the value assigned in the declaration, or the type's default-constructed value).

Example (object fields):

```cpp
// Person from above
const auto homer = rfl::json::read<Person>(R"({"first_name":"Homer"})").value();
// homer.last_name == "Simpson" (declared default)
// homer.town == "" (default-constructed)
```

Example (no field names / positional arrays):

DefaultVal also works when using rfl::NoFieldNames (positional JSON arrays). Omitted positions that correspond to DefaultVal fields get their default values:

```cpp
const auto homer = rfl::json::read<Person, rfl::NoFieldNames>(R"(["Homer"])").value();
// homer.first_name == "Homer"
// homer.last_name == "Simpson"
// homer.town == ""
```

## Fields without a valid default value

A struct that contains an `rfl::DefaultVal` is default-constructed before it is read, so that the fields missing from the input keep their default values. The fields found in the input are then replaced by the values read from it.

Some fields have no valid default value, for instance a validator that an empty vector does not satisfy:

```cpp
using Foods = rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>>;

struct Person {
  rfl::DefaultVal<std::string> name = "Homer Simpson";
  Foods favorite_foods;
};

const auto homer = rfl::json::read<Person>(R"({"favorite_foods":["donuts","pizza"]})");
```

`Person{}` throws, but `read` then constructs the struct again with the check of such a validator deferred until the input has been read. If `favorite_foods` is in the input, its invalid default value has been replaced. If it is missing, you get the error `Field named 'favorite_foods' not found.` Outside of `read`, `Person{}` still throws.

A field whose default value is invalid must not be missing, even where a missing field would otherwise get its default value, that is, for an `rfl::DefaultVal` or with the `rfl::DefaultIfMissing` processor:

```
Field named 'favorite_foods' not found, and its default value is invalid: Size validation failed: Value expected to be greater than or equal to 1, but got 0.
```

While the struct is constructed again, every validator that is default-constructed on the same thread keeps its invalid value instead of throwing, wherever it is: in a field, in a nested struct or container, in a temporary, or in a copy such as `rfl::DefaultVal<Foods> other_foods = favorite_foods;`. After reading, `read` checks the validators inside every field that is missing from the input, and fails with the error above if one of them is invalid. `read` looks into structs, containers, `std::optional`, smart pointers, variants, tuples, the wrappers of reflect-cpp and classes with a `reflection()` method. If a missing field holds a class it cannot look into, you get the error `Field named '...' not found, and its default value could not be checked: ...`.

If the struct holds fewer invalid validators than were default-constructed, an invalid value has been used up on the way, for instance a temporary in `int port = Port().value();`, or it is inside a class that `read` cannot look into. A copy can make up for a used-up value, though, and a default member initializer that reads a validator, as in `int count = favorite_foods->size();`, gets its invalid value without an error. A default member initializer that catches the exception of such a validator in order to fall back to another value gets no exception while validation is deferred, so the field must then be in the input. The struct cannot be read, and you get the error below, if an invalid value has been used up, or if `Person{}` throws for another reason, including a validator that is given an invalid value explicitly (as in `Foods favorite_foods = std::vector<std::string>();`):

```
Could not default-construct Person, which is required because it contains rfl::DefaultVal. Give every field a valid default value. Original error: Size validation failed: Value expected to be greater than or equal to 1, but got 0.
```

In all of these cases, you can give the field a valid default value:

```cpp
struct Person {
  rfl::DefaultVal<std::string> name = "Homer Simpson";
  Foods favorite_foods = std::vector<std::string>({"none"});
};
```

The field is still required. The default value is only used for a missing field if you pass the `rfl::DefaultIfMissing` processor.

All of this applies to every struct that is read with the `rfl::DefaultIfMissing` processor, whether it contains an `rfl::DefaultVal` or not. In JSON schemas, structs with `rfl::DefaultVal` keep their valid default values; any field whose default value holds an invalid validator or a class that cannot be checked gets no default value and is listed as required (unless `rfl::DefaultIfMissing` omits `required` for the whole schema). If an invalid value has been used up, the schema keeps no default values and marks those fields required. If `Person{}` throws for another reason, `rfl::json::to_schema<Person>()` throws that exception. If such a struct is nested inside another struct, the program ends with `std::terminate`, as it does without this feature.

An `rfl::NamedTuple` has no in-class initializers. A missing `rfl::DefaultVal<T>` field in it is set to `T{}`. If `T` has no default constructor, or `T{}` throws (for instance a validator that rejects its zero value), the field must be present in the input.

## When to use

Use `rfl::DefaultVal` when you want a field to be optional at the input side but still available as a value on the resulting object (no std::optional or pointer indirection). It is particularly useful for fields with sensible defaults (for example, a common last name, a default configuration value, or empty containers/strings).

## Notes

- The underlying type must be default-constructible to allow resetting via `rfl::Default` or when no explicit default is supplied.
- DefaultVal preserves normal read/write semantics; other fields that are not DefaultVal remain required unless expressed as optionals or handled by processors (e.g., `rfl::DefaultIfMissing`).

For more advanced control over when fields are considered missing and how defaults are applied, see the processors documentation (e.g., `rfl::DefaultIfMissing`).
