# Support for standard containers

The serialization and deserialization of containers is handled as follows:

`std::map<std::string, ...>`, `std::unordered_map<std::string, ...>`,
maps with any numeric types (integral and floating points) as keys,
and any objects where the `ReflectionType` is either a `std::string` or
a numeric type are treated as objects.

Consider the following map:

```cpp
std::map<std::string, std::string> homer;
homer["firstName"] = "Homer";
homer["lastName"] = "Simpson";
```

This will be represented as follows:

```json
{"firstName":"Homer","lastName":"Simpson"}
```

For JSON and other formats that use string map keys, the same is true for numeric types:

```cpp
std::map<int, std::string> homer;
homer[1] = "Homer";
homer[2] = "Simpson";
```

This will be represented as follows:

```json
{"1":"Homer","2":"Simpson"}
```

If you wrap your keys into a `rfl::Validator`, this will be treated as an object as well:

```cpp
using Between1and3 = rfl::Validator<int, rfl::Minimum<1>, rfl::Maximum<3>>;
std::map<Between1and3, std::string> homer;
homer[1] = "Homer";
homer[2] = "Simpson";
```

This will also be represented as follows:

```json
{"1":"Homer","2":"Simpson"}
```

All other supported standard containers
(other than `std::bitset`, `std::variant`, `std::optional`, `std::unique_ptr`, `std::shared_ptr` and `std::expected`)
will be represented as arrays. Containers for which the `value_type`
is a key-value-pair will be represented as arrays of pairs.

`std::expected` is an exception to this: it is serialized as its value type, or as an object with a single `error` field. Refer to the [std::expected](../expected) section for details.

`std::bitset<N>` uses the native representation in Cereal, YAS, CBOR, and
Boost.Serialization (packed in the first three, a string in Boost). Other
binary formats use `ceil(N / 8)` packed bytes: bit zero is the high bit of the
first byte, and unused low bits are zero. Other formats use exactly `N` binary
characters in `to_string()` order. The shared parsers reject incorrect lengths,
invalid characters, and nonzero padding; native adapters retain native validation.

## Typed map keys in binary formats

MessagePack, Cereal, YAS, and Boost.Serialization serialize the keys of the
maps described above as values of their original type. This applies to both
`std::map` and `std::unordered_map`, including numeric `ReflectionType` wrappers
such as `rfl::Validator`. Validation is applied when reading keys.

For example, `std::map<int, std::string>{{1, "Homer"}}` contains an integer key
in MessagePack, rather than the string `"1"`. The archive backends write the
entry count followed by each typed key and its value. No intermediate container
or conversion to/from decimal strings is needed. Floating-point keys retain
their precision instead of being rounded by `std::to_string`.

This changes the wire format for numeric keys in these backends. There is no
automatic detection or compatibility mode for previously serialized string
keys; readers and writers must use the same representation.

Other backends retain their existing representation. In particular, the CBOR
adapter currently reads through a JSON DOM with string object keys, so native
typed CBOR map keys are not supported by this change. Maps with compound keys
and multimaps continue to use arrays of pairs.
