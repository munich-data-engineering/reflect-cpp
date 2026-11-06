#ifndef RFL_XML_READER_HPP_
#define RFL_XML_READER_HPP_

#include <charconv>
#include <exception>
#include <optional>
#include <pugixml.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

#include "../Result.hpp"
#include "../always_false.hpp"
#include "../parsing/is_view_reader.hpp"

namespace rfl::xml {

/// Reader class for deserializing XML (Extensible Markup Language) data.
/// This class provides the interface for parsing XML format into C++ objects.
/// Uses the pugixml library for XML parsing. XML has a hierarchical structure
/// with elements (nodes) and attributes, both of which can be mapped to object fields.
struct Reader {
  /// Represents an XML array during deserialization.
  /// In XML, arrays are represented as multiple sibling elements with the same name.
  /// Wraps a pugixml node representing the first element in the array.
  struct XMLInputArray {
    XMLInputArray(pugi::xml_node _node) : node_(_node) {}
    pugi::xml_node node_;
  };

  /// Represents an XML object during deserialization.
  /// Wraps a pugixml node that can contain child elements and attributes.
  struct XMLInputObject {
    XMLInputObject(pugi::xml_node _node) : node_(_node) {}
    pugi::xml_node node_;
  };

  /// Represents a variant XML value during deserialization.
  /// Can hold either an XML node (element) or an XML attribute.
  /// This distinction is unique to XML format.
  struct XMLInputVar {
    XMLInputVar() : node_or_attribute_(pugi::xml_node()) {}
    XMLInputVar(pugi::xml_attribute _attr) : node_or_attribute_(_attr) {}
    XMLInputVar(pugi::xml_node _node) : node_or_attribute_(_node) {}
    std::variant<pugi::xml_node, pugi::xml_attribute> node_or_attribute_;
  };

  using InputArrayType = XMLInputArray;
  using InputObjectType = XMLInputObject;
  using InputVarType = XMLInputVar;

  /// Compile-time flag indicating whether type T has a custom constructor from XML.
  /// Currently not implemented for XML format (always false).
  template <class T>
  static constexpr bool has_custom_constructor = false;

  /// Retrieves an element from an XML array by index.
  /// In XML, arrays are represented as sibling elements with the same name.
  /// @param _idx The zero-based index of the element to retrieve
  /// @param _arr The XML array (first element of same-named siblings) to read from
  /// @return Result containing the element at the specified index, or an error if out of bounds
  rfl::Result<InputVarType> get_field_from_array(
      const size_t _idx, const InputArrayType& _arr) const noexcept {
    const auto name = _arr.node_.name();
    size_t i = 0;
    for (auto node = _arr.node_; node; node = node.next_sibling(name)) {
      if (i == _idx) {
        return InputVarType(node);
      }
    }
    return error("Index " + std::to_string(_idx) + " of of bounds.");
  }

  /// Retrieves a field from an XML object by name.
  /// Looks for a child element with the specified name.
  /// @param _name The name of the child element to retrieve
  /// @param _obj The XML object to read from
  /// @return Result containing the field value, or an error if field is not found
  rfl::Result<InputVarType> get_field_from_object(
      const std::string& _name, const InputObjectType _obj) const noexcept {
    const auto node = _obj.node_.child(_name.c_str());
    if (!node) {
      return error("Object contains no field named '" + _name + "'.");
    }
    return InputVarType(node);
  }

  /// Checks if an XML value is empty or null.
  /// @param _var The XML value (node or attribute) to check
  /// @return true if the value is empty/null, false otherwise
  bool is_empty(const InputVarType _var) const noexcept {
    const auto wrap = [](const auto& _node) { return !_node; };
    return std::visit(cast_as_node, _var.node_or_attribute_)
        .transform(wrap)
        .value_or(false);
  }

  /// Converts an XML value to a basic C++ type.
  /// Supports strings, booleans (as "true"/"false" or "1"/"0"), 
  /// floating-point numbers, and integers. Reads text content from nodes
  /// or values from attributes.
  /// @tparam T The target C++ type
  /// @param _var The XML value (node or attribute) to convert
  /// @return Result containing the converted value, or an error if conversion fails
  template <class T>
  rfl::Result<T> to_basic_type(const InputVarType _var) const noexcept {
    const auto get_value = [](const auto& _n) -> std::string {
      using Type = std::remove_cvref_t<decltype(_n)>;
      if constexpr (std::is_same<Type, pugi::xml_node>()) {
        return std::string(_n.child_value());
      } else {
        return std::string(_n.value());
      }
    };

    if constexpr (std::is_same<std::remove_cvref_t<T>, std::string>()) {
      return std::visit(get_value, _var.node_or_attribute_);

    } else if constexpr (std::is_same<std::remove_cvref_t<T>, bool>()) {
      const auto val = std::visit(get_value, _var.node_or_attribute_);
      if (val == "true" || val == "1") {
        return true;
      } else if (val == "false" || val == "0") {
        return false;
      } else {
        return error("Could not cast '" + val + "' to boolean.");
      }

    } else if constexpr (std::is_floating_point<std::remove_cvref_t<T>>()) {
      const auto str = std::visit(get_value, _var.node_or_attribute_);
      try {
        return static_cast<T>(std::stod(str));
      } catch (std::exception& e) {
        return error("Could not cast '" + std::string(str) +
                     "' to floating point value.");
      }

    } else if constexpr (std::is_integral<std::remove_cvref_t<T>>()) {
      const auto str = std::visit(get_value, _var.node_or_attribute_);
      T value;
      const auto [ptr, ec] =
          std::from_chars(str.data(), str.data() + str.size(), value);
      if (ec != std::errc() || ptr != str.data() + str.size()) {
        return error("Could not cast '" + std::string(str) + "' to integer.");
      }
      return value;

    } else {
      static_assert(rfl::always_false_v<T>, "Unsupported type.");
    }
  }

  /// Converts an XML value to an array.
  /// The node must be castable (not an attribute).
  /// @param _var The XML value to convert
  /// @return Result containing the array, or an error if value is an attribute
  rfl::Result<InputArrayType> to_array(const InputVarType _var) const noexcept {
    const auto wrap = [](const auto& _node) { return InputArrayType(_node); };
    return std::visit(cast_as_node, _var.node_or_attribute_).transform(wrap);
  }

  /// Reads all elements from an XML array using a provided array reader.
  /// Iterates through all sibling elements with the same name.
  /// @tparam ArrayReader Type that provides a read() method for processing elements
  /// @param _array_reader The reader object used to process each array element
  /// @param _arr The XML array to read from
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ArrayReader>
  std::optional<Error> read_array(const ArrayReader& _array_reader,
                                  const InputArrayType& _arr) const noexcept {
    const auto name = _arr.node_.name();
    for (auto node = _arr.node_; node; node = node.next_sibling(name)) {
      auto err = _array_reader.read(InputVarType(node));
      if (err) {
        return err;
      }
    }
    return std::nullopt;
  }

  /// Reads all fields from an XML object using a provided object reader.
  /// Processes child elements, attributes, and optionally the text content.
  /// The object reader's read() method is called for each child and attribute.
  /// If the reader is a view reader, also provides access to "xml_content" field.
  /// @tparam ObjectReader Type that provides a read() method for processing fields
  /// @param _object_reader The reader object used to process each field
  /// @param _obj The XML object to read from
  /// @return std::nullopt on success, or an Error if reading fails
  template <class ObjectReader>
  std::optional<Error> read_object(const ObjectReader& _object_reader,
                                   const InputObjectType& _obj) const noexcept {
    for (auto child = _obj.node_.first_child(); child;
         child = child.next_sibling()) {
      _object_reader.read(std::string_view(child.name()), InputVarType(child));
    }

    for (auto attr = _obj.node_.first_attribute(); attr;
         attr = attr.next_attribute()) {
      _object_reader.read(std::string_view(attr.name()), InputVarType(attr));
    }

    if constexpr (parsing::is_view_reader_v<ObjectReader>) {
      _object_reader.read(std::string_view("xml_content"),
                          InputVarType(_obj.node_));
    }

    return std::nullopt;
  }

  /// Converts an XML value to an object.
  /// The node must be castable (not an attribute).
  /// @param _var The XML value to convert
  /// @return Result containing the object, or an error if value is an attribute
  rfl::Result<InputObjectType> to_object(
      const InputVarType _var) const noexcept {
    const auto wrap = [](const auto& _node) { return InputObjectType(_node); };
    return std::visit(cast_as_node, _var.node_or_attribute_).transform(wrap);
  }

  /// Uses a type's custom constructor to deserialize from XML.
  /// Currently not implemented for XML format.
  /// @tparam T The type to construct
  /// @param _var The XML value to deserialize from
  /// @return Error indicating this feature is not yet implemented
  template <class T>
  rfl::Result<T> use_custom_constructor(
      const InputVarType _var) const noexcept {
    return error("TODO");
  }

 private:
  /// XML-only helper function. This is needed because XML distinguishes between
  /// nodes and attributes.
  static rfl::Result<pugi::xml_node> cast_as_node(
      const std::variant<pugi::xml_node, pugi::xml_attribute>&
          _node_or_attribute) {
    const auto cast = [](const auto& _n) -> Result<pugi::xml_node> {
      using Type = std::remove_cvref_t<decltype(_n)>;
      if constexpr (std::is_same<Type, pugi::xml_node>()) {
        return _n;
      } else {
        return error("Field '" + std::string(_n.name()) + "' is an attribute.");
      }
    };
    return std::visit(cast, _node_or_attribute);
  }
};

}  // namespace rfl::xml

#endif
