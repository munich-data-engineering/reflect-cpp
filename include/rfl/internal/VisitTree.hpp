#ifndef RFL_INTERNAL_VISITTREE_HPP_
#define RFL_INTERNAL_VISITTREE_HPP_

namespace rfl::internal {

struct VisitTree {
  /// Evaluates a visitor pattern using a tree-like structure.
  template <int _begin, int _end, class Visitor, class... Args>
  static inline auto visit(const auto& _v, const int _i, const Args&... _args) {
    static_assert(_end > _begin, "_end needs to be greater than _begin.");
    if constexpr (_end - _begin == 1) {
      return _v.template visit<_begin>(_args...);
    } else {
      constexpr int middle = (_begin + _end) / 2;
      if (_i < middle) {
        return visit<_begin, middle, Visitor>(_v, _i, _args...);
      } else {
        return visit<middle, _end, Visitor>(_v, _i, _args...);
      }
    }
  }
};

}  // namespace rfl::internal

#endif
