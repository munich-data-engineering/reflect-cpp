---
breadcrumbs: true
---

# Supporting your own format

In order to support your own serialization format, you need to implement a reader and a writer on top of whatever serialization libary
you have chosen.

The reader and writer constitute a thin layer between the serialization library of your choice and reflect-cpp.

The requirements are laid down in the concepts [IsReader](https://github.com/munich-data-engineering/reflect-cpp/blob/main/include/rfl/parsing/IsReader.hpp) and
[IsWriter](https://github.com/munich-data-engineering/reflect-cpp/blob/main/include/rfl/parsing/IsWriter.hpp) and also documented below.

Using the reader and the writer you can define your parser like this:

```cpp
template <class T>
using YourParser = rfl::parsing::Parser<YourReader, YourWriter, T>;
```

This can then be used to implement a `read` function and a `write` function:

```cpp
using InputVarType = typename YourReader::InputVarType;
using OutputVarType = typename YourWriter::OutputVarType;

template <class T>
rfl::Result<T> read(const std::string& _str) {
    // This should be supported by whatever library you are
    // using for your format.
    const InputVarType root = str_to_input_var(_str);

    // You can pass variables to the constructor, if necessary
    const auto r = Reader(...);

    return YourParser<T>::read(r, root);
}

template <class T>
std::string write(const T& _obj) {
    // You can pass variables to the constructor, if necessary
    auto w = Writer(...);

    OutputVarType var = Parser<T>::write(w, _obj);

    // This should be supported by whatever library you are
    // using for your format.
    return output_var_to_str(var);
}
```

In the following two sections, we will provide templates for your `Reader` and `Writer`.
You should probably just copy + paste this into your own code and fill in the blanks.

As a reference, you can take a look at how this is done for JSON: https://github.com/munich-data-engineering/reflect-cpp/tree/main/include/rfl/json

## Implementing your own writer

Because writers are somewhat simpler, we will start with them.

Any Writer needs to define the following types:

1) An `OutputArrayType`, which must be an array-like data structure.
2) An `OutputObjectType`, which must contain key-value pairs.
3) An `OutputVarType`, which must be able to represent either
   `OutputArrayType`, `OutputObjectType` or a basic type (bool, integral,
   floating point, std::string). We hesitate to call these "primitive types",
   because primitive types in C++ are defined as a slightly different group
   of types.

It also needs to support the following methods:

```cpp
struct Writer {
    using OutputArrayType = ...;
    using OutputObjectType = ...;
    using OutputVarType = ...;

  /// Sets an empty array as the root element of the document.
  /// Some serialization formats require you to pass the expected size in
  /// advance. If you are not working with such a format, you can ignore the
  /// parameter `_size`. Returns the new array for further modification.
  OutputArrayType array_as_root(const size_t _size) const noexcept;

  /// Sets an empty object as the root element of the document.
  /// Some serialization formats require you to pass the expected size in
  /// advance. If you are not working with such a format, you can ignore the
  /// parameter `_size`.
  /// Returns the new object for further modification.
  OutputObjectType object_as_root(const size_t _size) const noexcept;

  /// Sets a null as the root element of the document. Returns OutputVarType
  /// containing the null value.
  OutputVarType null_as_root() const noexcept;

  /// Sets a basic value (bool, numeric, string) as the root element of the
  /// document. Returns an OutputVarType containing the new value.
  template <class T>
  OutputVarType value_as_root(const T& _var) const noexcept;

  /// Adds an empty array to an existing array. Returns the new
  /// array for further modification.
  OutputArrayType add_array_to_array(const size_t _size,
                                     OutputArrayType* _parent) const noexcept;

  /// Adds an empty array to an existing object. The key or name of the field is
  /// signified by `_name`. Returns the new array for further modification.
  OutputArrayType add_array_to_object(
      const std::string_view& _name, const size_t _size,
      OutputObjectType* _parent) const noexcept;

  /// Adds an empty object to an existing array. Returns the new
  /// object for further modification.
  OutputObjectType add_object_to_array(
      const size_t _size, OutputArrayType* _parent) const noexcept;

  /// Adds an empty object to an existing object. The key or name of the field
  /// is signified by `_name`. Returns the new object for further modification.
  OutputObjectType add_object_to_object(
      const std::string_view& _name, const size_t _size,
      OutputObjectType* _parent) const noexcept;

  /// Adds a basic value (bool, numeric, string) to an array. Returns an
  /// OutputVarType containing the new value.
  template <class T>
  OutputVarType add_value_to_array(const T& _var,
                                   OutputArrayType* _parent) const noexcept;

  /// Adds a basic value (bool, numeric, string) to an existing object. The key
  /// or name of the field is signified by `name`. Returns an
  /// OutputVarType containing the new value.
  template <class T>
  OutputVarType add_value_to_object(const std::string_view& _name, const T& _var,
                                    OutputObjectType* _parent) const noexcept;

  /// Adds a null value to an array. Returns an
  /// OutputVarType containing the null value.
  OutputVarType add_null_to_array(OutputArrayType* _parent) const noexcept;

  /// Adds a null value to an existing object. The key
  /// or name of the field is signified by `name`. Returns an
  /// OutputVarType containing the null value.
  OutputVarType add_null_to_object(const std::string_view& _name,
                                   OutputObjectType* _parent) const noexcept;

  /// Signifies to the writer that we do not want to add any further elements to
  /// this array. Some serialization formats require this. If you are working
  /// with a serialization format that doesn't, just leave the function empty.
  void end_array(OutputArrayType* _arr) const noexcept;

  /// Signifies to the writer that we do not want to add any further elements to
  /// this object. Some serialization formats require this. If you are working
  /// with a serialization format that doesn't, just leave the function empty.
  void end_object(OutputObjectType* _obj) const noexcept;
};
```

## Implementing your own reader

Any Reader needs to define the following:

1) An `InputArrayType`, which must be an array-like data structure.
2) An `InputObjectType`, which must contain key-value pairs.
3) An `InputVarType`, which must be able to represent either
   `InputArrayType`, `InputObjectType` or a basic type (bool, integral,
   floating point, std::string).
4) A `static constexpr bool has_custom_constructor<T>`, that determines
   whether the class in question as a custom constructor, which might
   be called something like `from_json_obj(...)`. If you do not want to
   support this functionality, just set it to false.

It also needs to support the following methods:

```cpp
struct Reader {
    using InputArrayType = ...;
    using InputObjectType = ...;
    using InputVarType = ...;

    /// If you do not want to support custom constructors,
    /// just set this to false.
    template <class T>
    static constexpr bool has_custom_constructor = false;

    /// Retrieves a particular field from an array.
    /// Returns an rfl::Error if the index is out of bounds.
    /// If your format is schemaful, you do not need this.
    rfl::Result<InputVarType> get_field_from_array(
        const size_t _idx, const InputArrayType _arr) const noexcept {...}

    /// Retrieves a particular field from an object.
    /// Returns an rfl::Error if the field cannot be found.
    /// If your format is schemaful, you do not need this.
    rfl::Result<InputVarType> get_field_from_object(
        const std::string& _name, const InputObjectType& _obj) const noexcept {...}

    /// Determines whether a variable is empty (the NULL type).
    bool is_empty(const InputVarType& _var) const noexcept {...}

    /// Cast _var as a basic type (bool, integral,
    /// floating point, std::string).
    /// Returns an rfl::Error if it cannot be cast
    /// as that type
    template <class T>
    rfl::Result<T> to_basic_type(const InputVarType& _var) const noexcept {...}

    /// Casts _var as an InputArrayType.
    /// Returns an rfl::Error if `_var` cannot be cast as an array.
    rfl::Result<InputArrayType> to_array(const InputVarType& _var) const noexcept {...}

    /// Casts _var as an InputObjectType.
    /// Returns an rfl::Error if `_var` cannot be cast as an object.
    rfl::Result<InputObjectType> to_object(
        const InputVarType& _var) const noexcept {...}

    /// Iterates through an array and inserts the values into the array
    /// reader. See below for a more detailed explanation.
    template <class ArrayReader>
    std::optional<Error> read_array(const ArrayReader& _array_reader,
                                    const InputArrayType& _arr) const noexcept {...}

    /// Iterates through an object and inserts the key-value pairs into the object
    /// reader. See below for a more detailed explanation.
    template <class ObjectReader>
    std::optional<Error> read_object(const ObjectReader& _object_reader,
                                     const InputObjectType& _obj) const noexcept {...}

    /// Constructs T using its custom constructor. This will only be triggered if
    /// T was determined to have a custom constructor by
    /// static constexpr bool has_custom_constructor, as defined above.
    /// Returns an rfl::Error, if the custom constructor throws an exception.
    template <class T>
    rfl::Result<T> use_custom_constructor(
        const InputVarType& _var) const noexcept {
        // If you do not want to support this functionality,
        // just return this.
        return rfl::error("Not supported.");
    }
};
```

Of these methods, `read_array` and `read_object` probably require further explanation.

## `read_array`

`read_array` expects an `ArrayReader` class which might come in several forms. But all
of these forms have a method with the following signature:

```cpp
std::optional<Error> read(const InputVarType& _var) const noexcept;
```

Within your implementation of `read_array`, you must iterate through the array passed
to the function and then insert the resulting values into `array_reader.read`. If
`array_reader.read` returns an error, then you must return that error immediately.

## `read_object`

`read_object` expects an `ObjectReader` class which might come in several forms. But all
of these forms have a method with the following signature:

```cpp
void read(const std::string_view& _name,
          const InputVarType& _var) const noexcept;
```

Within your implementation of `read_object`, you must iterate through the object passed
to the function and then insert the resulting key-value-pairs into `object_reader.read`.

## Additional requirements for schemaful formats

Schemaful formats, like Apache Avro or Cap'n Proto,
are somewhat more complicated than schemaless ones. There are additional factors
to consider which do not apply schemaless formats:

1. Schemaful formats needs to differentiate between *objects*, for which
   the field names are known at compile time and *maps*, for which the
   field names are not known at compile time. In schemaless formats, there
   is no differentiation.

2. Schemaful formats needs an explicit union types. This also means that
   many of the problems we have with serializing `std::variant` which
   requires us to develop concepts like `rfl::TaggedUnion` simply do not
   apply to schemaful formats - the problem is already solved.

### Additional requirements for schemaful writers

Any schemaful reader additionally needs to define the following:

1) An `OutputMapType`, which must contain key-value pairs.
2) An `OutputUnionType`, which represents an explicit union.

```cpp
struct Writer {
  using OutputArrayType = ...;
  using OutputMapType = ...;
  using OutputObjectType = ...;
  using OutputUnionType = ...;
  using OutputVarType = ...;

  /// Sets an empty map as the root element of the document.
  /// Some serialization formats require you to pass the expected size in
  /// advance. If you are not working with such a format, you can ignore the
  /// parameter `size`. Returns the new array for further modification.
  OutputMapType map_as_root(const size_t _size) const noexcept;

  /// Sets an empty union as the root element of the document.
  OutputUnionType union_as_root() const noexcept;

  /// Adds an empty array to an existing map. Returns the new
  /// array for further modification.
  OutputArrayType add_array_to_map(const std::string_view& _name,
                                   const size_t _size,
                                   OutputMapType* _parent) const noexcept;

  /// Adds an empty array to an existing union.
  /// The index refers to the index of the element in the union.
  /// Returns the new array for further modification.
  OutputArrayType add_array_to_union(const size_t _index, const size_t _size,
                                     OutputUnionType* _parent) const noexcept;

  /// Adds an empty map to an existing array. Returns the new
  /// map for further modification.
  OutputMapType add_map_to_array(const size_t _size,
                                 OutputArrayType* _parent) const noexcept;

  /// Adds an empty map to an existing map. The key or name of the field
  /// is signified by `name`. Returns the new map for further modification.
  OutputMapType add_map_to_map(const std::string_view& _name,
                               const size_t _size,
                               OutputMapType* _parent) const noexcept;

  /// Adds an empty map to an existing object. The key or name of the field
  /// is signified by `name`. Returns the new map for further modification.
  OutputMapType add_map_to_object(const std::string_view& _name,
                                  const size_t _size,
                                  OutputObjectType* _parent) const noexcept;

  /// Adds an empty map to an existing union.
  /// The index refers to the index of the element in the union.
  /// Returns the new map for further modification.
  OutputMapType add_map_to_union(const size_t _index, const size_t _size,
                                 OutputUnionType* _parent) const noexcept;

  /// Adds an empty object to an existing map. The key or name of the field
  /// is signified by `name`. Returns the new object for further modification.
  OutputObjectType add_object_to_map(const std::string_view& _name,
                                     const size_t _size,
                                     OutputMapType* _parent) const noexcept;

  /// Adds an empty object to an existing union.
  /// The index refers to the index of the element in the union.
  /// Returns the new object for further modification.
  OutputObjectType add_object_to_union(const size_t _index, const size_t _size,
                                       OutputUnionType* _parent) const noexcept;

  /// Adds an empty union to an existing array. Returns the new
  /// union for further modification.
  OutputUnionType add_union_to_array(OutputArrayType* _parent) const noexcept;

  /// Adds an empty union to an existing map. The key or name of the field
  /// is signified by `name`. Returns the new union for further modification.
  OutputUnionType add_union_to_map(const std::string_view& _name,
                                   OutputMapType* _parent) const noexcept;

  /// Adds an empty union to an existing object. The key or name of the field
  /// is signified by `name`. Returns the new union for further modification.
  OutputUnionType add_union_to_object(const std::string_view& _name,
                                      OutputObjectType* _parent) const noexcept;

  /// Adds an empty union to an existing union.
  /// The index refers to the index of the element in the union.
  /// Returns the new union for further modification.
  OutputUnionType add_union_to_union(const size_t _index,
                                     OutputUnionType* _parent) const noexcept;

  /// Adds a null value to a map. Returns an
  /// OutputVarType containing the null value.
  OutputVarType add_null_to_map(const std::string_view& _name,
                                OutputMapType* _parent) const noexcept;

  /// Adds a null value to a union. Returns an
  /// OutputVarType containing the null value.
  OutputVarType add_null_to_union(const size_t _index,
                                  OutputUnionType* _parent) const noexcept;

  /// Adds a basic value (bool, numeric, string) to an existing map. The key
  /// or name of the field is signified by `name`. Returns an
  /// OutputVarType containing the new value.
  template <class T>
  OutputVarType add_value_to_map(const std::string_view& _name, const T& _var,
                                 OutputMapType* _parent) const noexcept;

  /// Adds a basic value (bool, numeric, string) to an existing union. The key
  /// or name of the field is signified by `name`. Returns an
  /// OutputVarType containing the new value.
  template <class T>
  OutputVarType add_value_to_union(const size_t _index, const T& _var,
                                   OutputUnionType* _parent) const noexcept;

  /// Signifies to the writer that we do not want to add any further elements to
  /// this map. Some serialization formats require this. If you are working
  /// with a serialization format that doesn't, just leave the function empty.
  void end_map(OutputMapType* _obj) const noexcept;

};
```

### Additional requirements for schemaful readers

Any schemaful reader additionally needs to define the following:

1) An `InputMapType`, which must contain key-value pairs.
2) An `InputUnionType`, which represents an explicit union.

```cpp
struct Reader {
  using InputArrayType = ...;
  using InputObjectType = ...;
  using InputMapType = ...;
  using InputUnionType = ...;
  using InputVarType = ...;

  /// A schemaful reader needs to differentiate between objects, for which
  /// the field names are known at compile time and maps, for which the
  /// field names are not known at compile time.
  rfl::Result<InputMapType> to_map(const InputVarType& _var) const noexcept;

  /// read_map works exactly the same as read_object in schemaless formats.
  template <class MapReader>
  std::optional<Error> read_map(const MapReader& _map_reader,
                                const InputMapType& _map) const noexcept;

  /// A schemaful reader needs an explicit union type.
  rfl::Result<InputUnionType> to_union(const InputVarType& _var) const noexcept;

  /// read_union needs to be able to take an InputUnionType and return the corresponding
  /// variant (like std::variant or rfl::Variant).
  template <class VariantType, class UnionReaderType>
  rfl::Result<VariantType> read_union(
      const InputUnionType& _union) const noexcept;
};
```


### Optional typed map keys

Backends that preserve map key types provide explicit map-entry APIs for
writing and reading. They are detected independently by
`SupportsTypedMapWriter` and `SupportsTypedMapReader`.

A writer defines an `OutputMapEntryType` context and the following methods:

```cpp
OutputMapEntryType map_entry(OutputMapType* map) const;
OutputArrayType add_array_to_map_entry(size_t size, OutputMapEntryType*) const;
OutputObjectType add_object_to_map_entry(size_t size, OutputMapEntryType*) const;
OutputVarType add_null_to_map_entry(OutputMapEntryType*) const;

template <class T>
OutputVarType add_value_to_map_entry(const T& value, OutputMapEntryType*) const;

// Additionally required for schemaful writers:
OutputMapType add_map_to_map_entry(size_t size, OutputMapEntryType*) const;
OutputUnionType add_union_to_map_entry(OutputMapEntryType*) const;
```

Schemaless writers use `OutputObjectType*` instead of `OutputMapType*` for
`map_entry`. The map is already open: `MapParser` uses the normal map header
for schemaful backends, or the object header for schemaless backends, and closes
it after all entries. For each entry, it creates a context and passes it through
`Parent::MapEntry` to the ordinary key and value parsers, in that order.
The context does not introduce an array or a pair-length prefix. Streaming
backends can use an empty context; other backends can retain their entry state
in it. The `add_*_to_map_entry` methods emit only the requested key or value.

The corresponding reader provides:

```cpp
template <class MapReader>
std::optional<Error> read_typed_map(const MapReader& reader,
                                  const InputMapType& map) const noexcept;
```

Schemaless readers use `InputObjectType` instead of `InputMapType`. For each
entry, call `reader.read(key, value)` with two `InputVarType` values and return
immediately if it returns an error. The shared `TypedMapReader` parses the key
before the value, so sequential archives may pass two handles to the same
archive. Backend traversal handles the map entry count and boundaries.

Schemaful readers can provide either `read_map` for string keys or
`read_typed_map` for typed keys. They do not need to implement both. Backends
without the typed-map interfaces retain string-key serialization. Objects
representing struct fields continue to use their existing field-name behavior.
