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

## All fields need a valid default value

A struct that contains an `rfl::DefaultVal` is default-constructed before it is read. The fields found in the input are then written into it. This means that every field of the struct needs a valid default value, even a required field that is in the input.

This matters when the default value of a field's type is not valid, for instance when the field has a validator:

```cpp
struct Person {
  rfl::DefaultVal<std::string> name = "Homer Simpson";
  rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>> favorite_foods;
};

// Fails, even though favorite_foods is in the input.
rfl::json::read<Person>(R"({"favorite_foods":["donuts","pizza"]})");
```

An empty vector does not satisfy `rfl::Size<rfl::Minimum<1>>`, so `Person{}` throws an exception and you get the following error:

```
Could not default-construct Person, which is required because it contains rfl::DefaultVal. Give every field a valid default value. Original error: Size validation failed: Value expected to be greater than or equal to 1, but got 0.
```

To avoid this, give the field a valid default value:

```cpp
struct Person {
  rfl::DefaultVal<std::string> name = "Homer Simpson";
  rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>> favorite_foods =
      std::vector<std::string>({"none"});
};
```

The field is still required. If `favorite_foods` is missing in the input, you get the error `Field named 'favorite_foods' not found.` The default value is only used for a missing field if you pass the `rfl::DefaultIfMissing` processor.

The same restriction applies to every struct that is read with the `rfl::DefaultIfMissing` processor, whether it contains an `rfl::DefaultVal` or not.

## When to use

Use `rfl::DefaultVal` when you want a field to be optional at the input side but still available as a value on the resulting object (no std::optional or pointer indirection). It is particularly useful for fields with sensible defaults (for example, a common last name, a default configuration value, or empty containers/strings).

## Notes

- The underlying type must be default-constructible to allow resetting via `rfl::Default` or when no explicit default is supplied.
- DefaultVal preserves normal read/write semantics; other fields that are not DefaultVal remain required unless expressed as optionals or handled by processors (e.g., `rfl::DefaultIfMissing`).

For more advanced control over when fields are considered missing and how defaults are applied, see the processors documentation (e.g., `rfl::DefaultIfMissing`).
