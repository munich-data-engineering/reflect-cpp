# `std::variant`, `rfl::Variant` and `rfl::TaggedUnion`

## `rfl::Variant` (untagged)

Sometimes you know that the JSON object can be one of several alternatives. For example,
you might have several shapes like `Circle`, `Rectangle` or `Square`. For these kind of 
cases, the C++ standard library contains `std::variant` and it is supported by reflect-cpp. 

However, we recommend you use `rfl::Variant` instead.

`rfl::Variant` behaves just like `std::variant`, but it compiles considerably faster,
particularly for variants with many alternatives.

You can use the functions `rfl::get`, `rfl::get_if`, `rfl::holds_alternative`, `rfl::visit`, 
`rfl::variant_alternative_t` and `rfl::variant_size_v` to access the variant and they work
the same way as their equivalents in the standard library.

```cpp
struct Circle {
  double radius;
};

struct Rectangle {
  double height;
  double width;
};

struct Square {
  double width;
};

using Shapes = rfl::Variant<Circle, Rectangle, Square>;

const Shapes r = Rectangle{.height = 10, .width = 5};

const auto json_string = rfl::json::write(r);

const auto r2 = rfl::json::read<Shapes>(json_string);
```

This code will compile just fine and work as intended. However, there are
several problems with this:

1) It is in inefficient: The parser has to read the fields for all of the different alternatives until it can't find a required field in the JSON object. It will then move on to the next alternative.
2) It leads to confusing error messages: If none of the alternatives can be matched, you will get an error message telling you why each of the alternatives couldn't be matched. Such error messages are very long-winding and hard to read.
3) It is dangerous. Imagine we had written `rfl::Variant<Circle, Square, Rectangle>` instead of `rfl::Variant<Circle, Rectangle, Square>`. This would mean that `Rectangle` could never be matched, because the fields in `Square` are a subset of `Rectangle`. This leads to very confusing bugs.

## Automatic tags

The easiest way to solve this problem is to simply add tags automatically. You can do so by using `rfl::AddTagsToVariants`:

```cpp
const auto json_string = rfl::json::write<rfl::AddTagsToVariants>(r);

const auto r2 = rfl::json::read<Shapes, rfl::AddTagsToVariants>(json_string);
```

Please refer to the section on processors in this documentation for more information.

## Automatic tags with full namespaces

In some cases, you might have struct name conflicts between different namespaces, or types that would generate identical tags when namespaces are removed. For these situations, you can use `rfl::AddNamespacedTagsToVariants` instead:

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

const Messages msg = Error::Message{.error = "Something went wrong", .error_id = 404};

const auto json_string = rfl::json::write<rfl::AddNamespacedTagsToVariants>(msg);
```

This generates JSON with fully qualified type names:

```json
{"Error::Message":{"error":"Something went wrong","error_id":404}}
```

The key differences between `rfl::AddTagsToVariants` and `rfl::AddNamespacedTagsToVariants`:

- `rfl::AddTagsToVariants` uses just the struct name (e.g., `"Message"`)
- `rfl::AddNamespacedTagsToVariants` uses the full namespaced name (e.g., `"Error::Message"`)
- Both respect custom tags defined with `using Tag = rfl::Literal<"custom_name">`
- Use the namespaced version when you are using `rfl::Generic` or need to distinguish between types in different namespaces and don't want to manually tag them as above

## `rfl::TaggedUnion` (internally tagged)

Another way to solve this problem is to add a tag inside the class. That is why we have provided a helper class for these purposes: `rfl::TaggedUnion`.

TaggedUnions use the name of the struct as an identifying tag. It will then try to take that field from the JSON object, match it to the correct alternative and then only parse the correct alternative.

We will now rewrite the example from above using `rfl::TaggedUnion`:

```cpp
struct Circle {
  double radius;
};

struct Rectangle {
  double height;
  double width;
};

struct Square {
  double width;
};

// Now you tell rfl::TaggedUnion that you want it to write the name
// of the struct into an extra field called "shape".
using Shapes = rfl::TaggedUnion<"shape", Circle, Square, Rectangle>;

const Shapes r = Rectangle{.height = 10, .width = 5};

const auto json_string = rfl::json::write(r);

const auto r2 = rfl::json::read<Shapes>(json_string);
```

The resulting JSON looks like this:
```json
{"shape":"Rectangle","height":10.0,"width":5.0}
```

Because the tag is inside the JSON object, this is called *internally tagged*. 
It is the standard in Python's [pydantic](https://docs.pydantic.dev/latest/api/standard_library_types/#union).

It is also possible to set the tag explicitly:

```cpp
struct Circle {
  using Tag = rfl::Literal<"circle", "Circle">;
  double radius;
};

struct Rectangle {
  using Tag = rfl::Literal<"rectangle", "Rectangle">;
  double height;
  double width;
};

struct Square {
  using Tag = rfl::Literal<"square", "Square">;
  double width;
};

using Shapes = rfl::TaggedUnion<"shape", Circle, Square, Rectangle>;

const Shapes r = Rectangle{.height = 10, .width = 5};

const auto json_string = rfl::json::write(r);

const auto r2 = rfl::json::read<Shapes>(json_string);
```

The JSON generated by `rfl::json::write` looks like this:

```json
{"shape":"rectangle","height":10.0,"width":5.0}
```

However, `rfl::json::read` would also accept this:

```json
{"shape":"Rectangle","height":10.0,"width":5.0}
```

If the behavior of your program depends on the value the user has decided to pass, then
you can also set the tag as a field explicitly. 

For instance, if it somehow makes a difference
whether the JSON contains "Rectangle" or "rectangle", you can do the following:

```cpp
struct Circle {
  rfl::Literal<"circle", "Circle"> shape;
  double radius;
};

struct Rectangle {
  rfl::Literal<"rectangle", "Rectangle"> shape;
  double height;
  double width;
};

struct Square {
  rfl::Literal<"square", "Square"> shape;
  double width;
};

using Shapes = rfl::TaggedUnion<"shape", Circle, Square, Rectangle>;

const Shapes r = Rectangle{
  .shape = rfl::Literal<"rectangle", "Rectangle">::make<"Rectangle">(),
  .height = 10,
  .width = 5};

const auto json_string = rfl::json::write(r);

const auto r2 = rfl::json::read<Shapes>(json_string);
```

Note that in this case the type of the field `shape` MUST be `rfl::Literal`.
Also note that this is exactly how tagged unions work in Pydantic. When you 
use the `rfl::NoFieldNames` processor, the tag MUST always be the first entry 
of the array.

## `std::variant` or `rfl::Variant` (externally tagged)

Another approach is to add external tags. 

You can do that using `rfl::Field`:

```cpp
using TaggedVariant = rfl::Variant<rfl::Field<"option1", Type1>, rfl::Field<"option2", Type2>, ...>;
```

The parser can now figure this out and will only try to parse the field that was indicated by the field name.
Duplicate field names will lead to compile-time errors.

We can rewrite the example from above:

```cpp
// Circle, Rectangle and Square are the same as above.
using Shapes = rfl::Variant<rfl::Field<"circle", Circle>,
                            rfl::Field<"rectangle", Rectangle>,
                            rfl::Field<"square", Square>>;

const Shapes r =
    rfl::make_field<"rectangle">(Rectangle{.height = 10, .width = 5});

const auto json_string = rfl::json::write(r);

const auto r2 = rfl::json::read<Shapes>(json_string);
```

The resulting JSON looks like this:
```json
{"rectangle":{"height":10.0,"width":5.0}}
```

Because the tag is external, this is called *externally tagged*. It is the standard in Rust's [serde-json](https://serde.rs/enum-representations.html).

## `std::monostate`

`std::monostate` is serialized as `null`. A `std::variant` or `rfl::Variant` whose
first alternative is `std::monostate` behaves like `std::optional`: as a field, it is
omitted when it holds `std::monostate` and a missing field is read as `std::monostate`.

```cpp
struct Order {
  std::variant<std::monostate, std::string, int> price;
};

rfl::json::write(Order{.price = std::monostate{}});  // {}
rfl::json::write(Order{.price = 2});                 // {"price":2}
```

Both `{}` and `{"price":null}` are read as `std::monostate`.

As with `std::optional`, passing `rfl::NoOptionals` writes the field as `null`
(`{"price":null}`) and makes it required on read.


## The visitor pattern

In C++, the idiomatic way to handle `std::variant`, `rfl::Variant` and `rfl::TaggedUnion` is the [visitor pattern](https://en.cppreference.com/w/cpp/utility/variant/visit).

For instance, the externally tagged `rfl::Variant` from the example above could be handled like this:

```cpp
using Shapes = rfl::Variant<rfl::Field<"circle", Circle>,
                            rfl::Field<"rectangle", Rectangle>,
                            rfl::Field<"square", Square>>;

const Shapes my_shape =
    rfl::make_field<"rectangle">(Rectangle{.height = 10, .width = 5});

const auto handle_shapes = [](const auto& field) {
  using Name = typename std::decay_t<decltype(field)>::Name;
  if constexpr (std::is_same<Name, rfl::Literal<"circle">>()) {
     std::cout << is circle, radius: << field.value().radius() << std::endl;
  } else if constexpr (std::is_same<Name, rfl::Literal<"rectangle">>()) {
     std::cout << is rectangle, width: << field.value().width() << ", height: " << field.value().height() << std::endl;
  } else if constexpr (std::is_same<Name, rfl::Literal<"square">>()) {
     std::cout << is square, width: << field.value().width() << std::endl;
  } else {
    // reflect-cpp also provides this very useful helper that ensures
    // at compile-time that you didn't forget anything.
    static_assert(rfl::always_false_v<Type>, "Not all cases were covered.");
  }
};

rfl::visit(handle_shapes, my_shape); // OK

my_shape.visit(handle_shapes); // also OK

```

You can also apply `rfl::visit` to `rfl::TaggedUnion`. The underlying `rfl::Variant` can be
retrieved using `.variant()`:

```cpp
using Shapes = rfl::TaggedUnion<"shape", Circle, Square, Rectangle>;

const Shapes my_shape = Rectangle{.height = 10, .width = 5};

const auto handle_shapes = [](const auto& s) {
  using Type = std::decay_t<decltype(s)>;
  if constexpr (std::is_same<Type, Circle>()) {
     std::cout << is circle, radius: << s.radius() << std::endl;
  } else if constexpr (std::is_same<Type, Rectangle>()) {
     std::cout << is rectangle, width: << s.width() << ", height: " << s.height() << std::endl;
  } else if constexpr (std::is_same<Type, Square>()) {
     std::cout << is square, width: << s.width() << std::endl;
  } else {
    static_assert(rfl::always_false_v<Type>, "Not all cases were covered.");
  }
};

rfl::visit(handle_shapes, my_shape); // OK

my_shape.visit(handle_shapes); // also OK
```
