# Processors

Processors can be used to apply transformations to struct serialization and deserialization.

For instance, C++ [usually](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rl-camel) uses `snake_case`, but JSON uses `camelCase`. One way to handle this is `rfl::Rename`, but a more automated way would be to use a *processor*:

```cpp
struct Person {
    std::string first_name;
    std::string last_name;
    std::vector<Person> children;
};

const auto homer =
    Person{.first_name = "Homer",
           .last_name = "Simpson",
           .age = 45};

const auto json_string =
  rfl::json::write<rfl::SnakeCaseToCamelCase>(homer);

const auto homer2 =
  rfl::json::read<Person, rfl::SnakeCaseToCamelCase>(json_string).value();
```

The resulting JSON string looks like this:

```json
{"firstName":"Homer","lastName":"Simpson","age":45}
```

## Supported processors

reflect-cpp currently supports the following processors:

- `rfl::AddStructName`
- `rfl::AddTagsToVariants`
- `rfl::AddNamespacedTagsToVariants`
- `rfl::AllowRawPtrs`
- `rfl::DefaultIfMissing`
- `rfl::DurationsAsCount`
- `rfl::EnumNamesOnly`
- `rfl::FailFast`
- `rfl::NoExtraFields`
- `rfl::NoFieldNames`
- `rfl::NoOptionals`
- `rfl::StringifyMapKeys`
- `rfl::UnderlyingEnums`
- `rfl::SnakeCaseToCamelCase`
- `rfl::SnakeCaseToPascalCase`

### `rfl::FailFast`

By default, reflect-cpp collects errors from multiple object fields.
`rfl::FailFast` reports the first field error and skips decoding the remaining
fields of that object. It also applies to nested structs and named tuples,
including structs stored in containers, across serialization formats.

```cpp
const auto result = rfl::yas::read<MyStruct, rfl::FailFast>(bytes);
```

This reduces error bookkeeping and can improve successful read performance.
It does not change the serialized representation. Existing calls without the
processor retain aggregated errors. Readers may still traverse input after a
field failure, and document parsing or structural errors can take precedence.
Variant matching and other parsing outside object fields keep their existing
behavior.

### `rfl::AddStructName`

It is also possible to add the struct name as an additional field, like this:

```cpp
const auto json_string =
  rfl::json::write<rfl::AddStructName<"type">>(homer);

const auto homer2 =
  rfl::json::read<Person, rfl::AddStructName<"type">>(json_string).value();
```

The resulting JSON string looks like this:

```json
{"type":"Person","first_name":"Homer","last_name":"Simpson","age":45}
```

### `rfl::AddTagsToVariants`

This processor automatically adds tags to variants. Consider the following example:

```cpp
struct button_pressed_t {};

struct button_released_t {};

struct key_pressed_t {
  char key;
};

using my_event_type_t =
    std::variant<button_pressed_t, button_released_t, key_pressed_t, int>;
```

The problem here is that `button_pressed_t` and `button_released_t` virtually look
indistinguishable when they are serialized. The will both be serialized to `{}`.

But you can add this processor to automatically add tags and avoid the problem:

```cpp
const auto vec = std::vector<my_event_type_t>(
  {button_pressed_t{}, button_released_t{}, key_pressed_t{'c'}, 3});

const auto json_string = rfl::json::write<rfl::AddTagsToVariants>(vec);

rfl::json::write<std::vector<my_event_type_t>, rfl::AddTagsToVariants>(json_string);
```

`vec` will now be serialized as follows:

```json
[{"button_pressed_t":{}},{"button_released_t":{}},{"key_pressed_t":{"key":99}},{"int":3}]
```

You can also set your own custom tags like this:

```cpp
struct key_pressed_t {
  using Tag = rfl::Literal<"your_custom_tag">;
  char key;
};
```

`key_pressed_t` will now be serialized as follows:

```json
{"your_custom_tag":{"key":99}}
```

Note that there are other ways to address problems like this, for instance `rfl::TaggedUnion`.
Please refer to the relevant sections of the documentation.

### `rfl::AddNamespacedTagsToVariants`

This processor is similar to `rfl::AddTagsToVariants`, but instead of using just the struct name as the tag, it uses the full namespaced type name. This is particularly useful when you have:

1. Structs with the same name in different namespaces
2. Structs with `rfl::Generic` fields that would otherwise create naming conflicts

#### Use-case: Structs with the same name in different namespaces

Consider this example where `rfl::AddTagsToVariants` would fail:

```cpp
namespace Result {
  struct Message {
    std::string result;
  };
}

namespace Error {
  struct Message {
    std::string error;
    int error_id;
  };
}

using Messages = std::variant<Result::Message, Error::Message>;

const auto msgs = std::vector<Messages>{
  Result::Message{.result = "success"},
  Error::Message{.error = "failure", .error_id = 404}
};

// This would cause problems with rfl::AddTagsToVariants because both
// structs have the same name "Message"

// But this works perfectly:
const auto json_string = rfl::json::write<rfl::AddNamespacedTagsToVariants>(msgs);
const auto msgs2 = rfl::json::read<std::vector<Messages>, rfl::AddNamespacedTagsToVariants>(json_string);
```

The resulting JSON includes the full namespace path:

```json
[
  {"Result::Message": {"result": "success"}},
  {"Error::Message": {"error": "failure", "error_id": 404}}
]
```

#### Use-case: Structs with `rfl::Generic` fields that would otherwise create naming conflicts

Another use case is with `rfl::Generic` fields, where multiple structs contain generic fields that would otherwise create naming conflicts:

```cpp
struct APIResult {
  rfl::Generic result;  // Could be string, number, object, etc.
};

struct APIError {
  rfl::Generic error;   // Could be string, number, object, etc.
};

using APIResponse = std::variant<APIResult, APIError>;

const auto response = APIResult{.result = std::string("200")};

// Without namespaces, both Generic fields would have the same tag name.
// With namespaces, they get unique identifiers:
const auto json_string = rfl::json::write<rfl::AddNamespacedTagsToVariants>(response);
```

This generates:

```json
{"APIResult": {"result": {"std::string": "200"}}}
```

#### When to use `rfl::AddNamespacedTagsToVariants` vs `rfl::AddTagsToVariants`

- Use `rfl::AddTagsToVariants` when struct names are unique and you want shorter, cleaner tags
- Use `rfl::AddNamespacedTagsToVariants` when you have naming conflicts or need to distinguish between types in different namespaces
- Custom tags (using `Tag = rfl::Literal<"custom_name">`) work with both processors and are not affected by the namespace handling

### `rfl::AllowRawPtrs`

By default, reflect-cpp does not allow *reading into* raw pointers, `std::string_view` or `std::span`.
(*Writing from* raw pointers is never a problem.) This is because reading into raw pointers
means that the library will allocate memory that the user then has to manually delete. This can lead to misunderstandings and memory leaks.

You might want to consider using some alternatives, such as `std::unique_ptr`, `rfl::Box`,
`std::shared_ptr`, `rfl::Ref` or `std::optional`.
But if you absolutely have to use raw pointers, you can pass `rfl::AllowRawPtrs` to `read`:

```cpp
struct Person {
  rfl::Rename<"firstName", std::string> first_name;
  rfl::Rename<"lastName", std::string> last_name = "Simpson";
  std::vector<Person>* children;`
};

const auto person =
  rfl::json::read<Person, rfl::AllowRawPtrs>(json_str);
```

However, you must keep in mind that it is now **YOUR** responsibility
to clean up. Otherwise, there **WILL** be a memory leak.

```cpp
void delete_raw_pointers(const Person& _person) {
    if (!_person.children) {
        return;
    }
    for (const auto& child: _person.children) {
        delete_raw_pointers(child);
    }
    delete _person.children;
}

delete_raw_pointers(person);
```

`std::string_view` and `std::span` must be cleaned up using `delete[]`, like this:

```cpp
delete[] person.string_view.data();

if(!person.span.empty()) {
    delete[] person.span.data();
}
```

### `rfl::DefaultIfMissing`

The `rfl::DefaultIfMissing` processor is only relevant for reading data. For writing data, it will make no difference.

Usually, when fields are missing in the input data, this will lead to an error
(unless they are optional fields).
But if you pass the `rfl::DefaultIfMissing` processor, then missing fields will be
replaced by their default value.

For instance, consider the following struct:

```cpp
struct Person {
  std::string first_name;
  std::string last_name = "Simpson";
  std::string town;
};
```

Suppose you are reading a JSON like this:

```json
{"first_name":"Homer"}
```

```cpp
rfl::json::read<Person, rfl::DefaultIfMissing>(json_string);
```

Then the resulting struct will be equivalent to what you would
have gotten had you read the following JSON string:

```json
{"first_name":"Homer","last_name":"Simpson","town":""}
```

`last_name` and `town` have been replaced by the default values.
Because you have not passed a default value to town, the default value
of the type is used instead.

### `rfl::DurationsAsCount`

By default, `std::chrono::duration` types are written as an object containing
the count and the unit. By passing the processor `rfl::DurationsAsCount`,
they will be written and read as a plain count in the unit of the C++ type:

```cpp
struct MyStruct {
  std::chrono::milliseconds timeout;
};

rfl::json::write<rfl::DurationsAsCount>(
    MyStruct{.timeout = std::chrono::milliseconds(1500)});
```

The resulting JSON string looks like this:

```json
{"timeout":1500}
```

### `rfl::EnumNamesOnly`

By default, when reading an enum from a string, numeric values are accepted
in addition to the declared enumerator names, even when they do not
correspond to a declared enumerator. For instance, given this enum:

```cpp
enum class Color { red = 1, green = 2, blue = 3 };
```

reading `{"color":"2"}` will produce `Color::green`, and even
`{"color":"4"}` will succeed and produce the cast value `4`, although `4`
is not a declared enumerator.

If you want to reject numeric values and only accept the declared
enumerator names, pass the `rfl::EnumNamesOnly` processor to `read`:

```cpp
const auto circle =
  rfl::json::read<Circle, rfl::EnumNamesOnly>(json_str);
```

Now, `{"color":"2"}` will lead to an error:

```
Failed to parse field 'color': Invalid enum value: '2'. Must be one of [red, green, blue].
```

This processor only affects reading. Enum values that cannot be matched to
a declared name are still written as their integer representation.

### `rfl::NoExtraFields`

When reading an object and the object contains a field that cannot be
matched to any of the fields in the struct, that field is simply ignored.

However, when `rfl::NoExtraFields` is added to `read`, then such extra fields
will lead to an error.

This can be overriden by adding `rfl::ExtraFields` to the struct.

Example:

```cpp
struct Person {
  std::string first_name;
  std::string last_name = "Simpson";
};
```

```json
{"first_name":"Homer","last_name":"Simpson","extra_field":0}
```

If you call `rfl::json::read<Person>(json_string)`, then `extra_field` will
simply be ignored.

But if you call `rfl::json::read<Person, rfl::NoExtraFields>(json_string)`,
you will get an error.

However, suppose the struct looked like this:

```cpp
struct Person {
  std::string first_name;
  std::string last_name = "Simpson";
  rfl::ExtraFields<int> extras;
};
```

In this case, `rfl::json::read<Person, rfl::NoExtraFields>(json_string)`
will not fail, because `extra_field` would be included in `extras`.

### `rfl::NoFieldNames`

We can also remove the field names altogether:

```cpp
const auto json_string =
  rfl::json::write<rfl::NoFieldNames>(homer);

const auto homer2 =
  rfl::json::read<Person, rfl::NoFieldNames>(json_string).value();
```

The resulting JSON string looks like this:

```json
["Homer","Simpson",45]
```

This is particularly relevant for binary formats, which do not emphasize readability,
like msgpack or flexbuffers. Removing the field names can reduce the size of the
resulting bytestrings and significantly speed up read and write time,
depending on the dataset.

However, it makes it more difficult to maintain backwards compatability.

Note that `rfl::NoFieldNames` is not supported for BSON, TOML, XML, or YAML, due
to limitations of these formats.

### `rfl::NoOptionals`

As we have seen in the section on optional fields, when a `std::optional` is
`std::nullopt`, it is usually not written at all. But if you want them to be explicitly
written as `null`, you can use this processor. The same thing applies to `std::shared_ptr` and
`std::unique_ptr`.

```cpp
struct Person {
  std::string first_name;
  std::string last_name = "Simpson";
  std::optional<std::string> town = std::nullopt;
};

const auto homer = Person{.first_name = "Homer"};

rfl::json::write<rfl::NoOptionals>(homer);
```

The resulting JSON string looks like this:

```json
{"first_name":"Homer","last_name":"Simpson","town":null}
```

By default, `rfl::json::read` will accept both `"town":null` and just
leaving out the field `town`. However, if you want to require the field
`town` to be included, you can add `rfl::NoOptionals` to `read`:

```
rfl::json::read<Person, rfl::NoOptionals>(json_string);
```

### `rfl::UnderlyingEnums`

By passing the processor `rfl::UnderlyingEnums`, fields of the enum type will be written and read as integers

```cpp
enum class Color { red, green, blue, yellow };

struct Circle {
  float radius;
  Color color;
};

const auto circle = Circle{.radius = 2.0, .color = Color::green};

rfl::json::write<rfl::UnderlyingEnums>(circle);
```

The resulting JSON string looks like this:

```json
{"radius":2.0,"color":1}
```

### `rfl::SnakeCaseToCamelCase`

Please refer to the example above.

### `rfl::SnakeCaseToPascalCase`

If you want `PascalCase` instead of `camelCase`, you can use the appropriate processor:

```cpp
const auto json_string =
  rfl::json::write<rfl::SnakeCaseToPascalCase>(homer);

const auto homer2 =
  rfl::json::read<Person, rfl::SnakeCaseToPascalCase>(json_string).value();
```

The resulting JSON string looks like this:

```json
{"FirstName":"Homer","LastName":"Simpson","Age":45}
```

## Combining several processors

You can combine several processors:

```cpp
const auto json_string =
  rfl::json::write<rfl::SnakeCaseToCamelCase, rfl::AddStructName<"type">>(homer);

const auto homer2 =
  rfl::json::read<Person, rfl::SnakeCaseToCamelCase, rfl::AddStructName<"type">>(json_string).value();
```

The resulting JSON string looks like this:

```json
{"type":"Person","firstName":"Homer","lastName":"Simpson","age":45}
```

When you have several processors, it is probably more convenient to combine them like this:

```cpp
using Processors = rfl::Processors<
    rfl::SnakeCaseToCamelCase, rfl::AddStructName<"type">>;

const auto json_string = rfl::json::write<Processors>(homer);

const auto homer2 = rfl::json::read<Person, Processors>(json_string).value();
```

The resulting JSON string looks like this:

```json
{"type":"Person","firstName":"Homer","lastName":"Simpson","age":45}
```

## Writing your own processors

In principle, writing your own processors is not very difficult. You need to define a struct, which takes has a static method called `process` taking a named tuple as an input and then returning a modified named tuple. The `process` method should accept the type of the original struct as a template parameter.

```cpp
struct MyOwnProcessor {
  template <class StructType>
  static auto process(auto&& _named_tuple) {...}
};
```

### `rfl::StringifyMapKeys`

Use `rfl::StringifyMapKeys` to represent `std::map` and `std::unordered_map`
with enum keys or string-reflected keys as maps (JSON objects, YAML mappings),
instead of arrays of key/value pairs. This is opt-in and applies to nested maps
as well. Pass the processor to both `read` and `write`.

```cpp
enum class Colour { Red, Green, Blue };
using Dictionary = std::map<Colour, std::string>;

const Dictionary dictionary{{Colour::Red, "Rojo"}, {Colour::Blue, "Azul"}};
const auto json = rfl::json::write<rfl::StringifyMapKeys>(dictionary);
// {"Red":"Rojo","Blue":"Azul"}
const auto decoded =
    rfl::json::read<Dictionary, rfl::StringifyMapKeys>(json);
```

For any key type `T` with a custom `rfl::Reflector<T>` whose `ReflType` is
`std::string`, writing uses `Reflector<T>::from` and reading uses
`Reflector<T>::to`. This includes both enums and user-defined classes. A
reflector takes precedence over the built-in enum name conversion.
`Reflector::to` may return `rfl::Result<T>` or throw a `std::exception` to report
an invalid key. Writing requires `from`; reading requires `to` for non-enum
string-reflected keys. As with other string-keyed maps, distinct keys should
have distinct string representations to preserve round trips.

Without a custom reflector, enum keys use the ordinary enum name conversion.
`rfl::EnumNamesOnly` also applies to keys: it rejects numeric strings as enum
names. `rfl::UnderlyingEnums` retains the array representation for enums without
a reflector, since those keys serialize as integers. String reflectors still
take precedence when both processors are present. Reflectors with a non-string
`ReflType` retain their existing representation.

Existing string, numeric and `ReflectionType`-based map keys keep their behavior.
Multimaps remain arrays of pairs, preserving duplicate keys. JSON Schema generated
with the processor describes these additional maps as objects, just like
`std::map<std::string, T>`. The processor changes map keys only; enum values
elsewhere keep their ordinary representation.
