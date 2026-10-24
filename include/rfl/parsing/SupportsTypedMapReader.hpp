#ifndef RFL_PARSING_SUPPORTSTYPEDMAPREADER_HPP_
#define RFL_PARSING_SUPPORTSTYPEDMAPREADER_HPP_

#include <concepts>
#include <optional>

#include "../Result.hpp"

namespace rfl::parsing {

template <class R>
struct MockTypedMapReader {
  std::optional<Error> read(const typename R::InputVarType&,
                            const typename R::InputVarType&) const {
    return std::nullopt;
  }
};

/// A map reader that exposes keys and values without converting keys to names.
template <class R, class MapType>
concept SupportsTypedMapReader =
    requires(const R& r, const MapType& map, MockTypedMapReader<R> reader) {
      { r.read_typed_map(reader, map) } -> std::same_as<std::optional<Error>>;
    };

}  // namespace rfl::parsing

#endif
