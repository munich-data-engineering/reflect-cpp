#ifndef RFL_DEFAULT_HPP_
#define RFL_DEFAULT_HPP_

namespace rfl {

/// Helper class that can be passed to a field to trigger the default value of the type.
/// Used as a sentinel value to indicate that the default-constructed value should be used.
struct Default {};

/// Convenience constant for the Default type.
/// Can be used like: MyField field = rfl::default_value;
inline const auto default_value = Default{};

}  // namespace rfl

#endif
