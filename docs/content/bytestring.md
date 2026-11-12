# `rfl::Bytestring` 

`rfl::Bytestring` is a simple alias:

```cpp
namespace rfl {

using Bytestring = std::vector<std::byte>;

}
```

Bytestrings are supported by binary formats such as BSON, CBOR, flexbuffers and msgpack. 

JSON represents bytestrings as Base64-encoded strings, following the
[ProtoJSON convention](https://protobuf.dev/programming-guides/json/). Writing uses the standard Base64 alphabet with `=` padding. Reading
accepts both the standard and URL-safe alphabets, with or without padding.
Invalid characters (including whitespace), invalid lengths and malformed padding
return an error.

```cpp
#include <rfl.hpp>
#include <rfl/json.hpp>

const rfl::Bytestring data{std::byte{0x00}, std::byte{0x80}, std::byte{0xff}};
const auto json = rfl::json::write(data);  // R"("AID/")" (a JSON string)
const auto decoded = rfl::json::read<rfl::Bytestring>(json);
// decoded.value() == data
```

An empty bytestring is written as `""`. Bytestrings also work in object fields,
arrays and optional values. This conversion only applies to `rfl::Bytestring`
(`std::vector<std::byte>`); ordinary strings and other vector types keep their
existing JSON representation. JSON numbers and arrays are not accepted as
bytestrings.

The generated JSON Schema uses `"type": "string"` and
`"contentEncoding": "base64"`. Size validators check the decoded byte count at
runtime; they are not emitted as JSON string-length constraints.

Other textual formats do not support bytestrings.
