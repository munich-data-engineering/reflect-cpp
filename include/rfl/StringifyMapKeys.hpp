#ifndef RFL_STRINGIFYMAPKEYS_HPP_
#define RFL_STRINGIFYMAPKEYS_HPP_

namespace rfl {

/// Represents maps with string-reflected keys or enum names as string-keyed
/// maps. Without this processor these additional key types use arrays of pairs.
struct StringifyMapKeys {
  template <class StructType>
  static auto process(auto&& _named_tuple) {
    return _named_tuple;
  }
};

}  // namespace rfl

#endif
