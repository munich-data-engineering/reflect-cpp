#ifndef RFL_METAFIELD_HPP_
#define RFL_METAFIELD_HPP_

#include <string>

namespace rfl {

/// Contains compile-time meta-information about a struct field.
/// MetaField stores the name and type information of a struct field for introspection purposes.
/// This is used internally by the reflection system to provide field metadata at runtime.
class MetaField {
 public:
  /// Constructs a MetaField with the given name and type information.
  /// @param _name The name of the field as it appears in the struct
  /// @param _type A string describing the type of the field
  MetaField(const std::string& _name, const std::string& _type)
      : name_(_name), type_(_type) {}

  /// Destructor.
  ~MetaField() = default;

  /// Returns the name of the field.
  /// @return The field name as it appears in the struct
  const std::string& name() const { return name_; }

  /// Returns the type information of the field.
  /// @return A string describing the field's type
  const std::string& type() const { return type_; }

 private:
  /// The name of the field.
  std::string name_;

  /// The type information of the field.
  std::string type_;
};

}  // namespace rfl

#endif  // RFL_TAGGEDUNION_HPP_
