#include "rfl/yaml/Reader.hpp"

#include <vector>

namespace rfl::yaml {

namespace {

std::vector<YAML::Node>& active_nodes() noexcept {
  static thread_local std::vector<YAML::Node> nodes;
  return nodes;
}

bool is_active_node(const std::vector<YAML::Node>& _nodes,
                    const YAML::Node& _node) noexcept {
  if (!_node.IsDefined()) {
    return false;
  }
  for (const auto& active : _nodes) {
    if (active.is(_node)) {
      return true;
    }
  }
  return false;
}

}  // namespace

Reader::ActiveNodeGuard::ActiveNodeGuard(const YAML::Node& _node)
    : is_cycle_(false), pushed_(false) {
  auto& nodes = active_nodes();
  if (is_active_node(nodes, _node)) {
    is_cycle_ = true;
    return;
  }
  if (_node.IsDefined()) {
    nodes.push_back(_node);
    pushed_ = true;
  }
}

Reader::ActiveNodeGuard::~ActiveNodeGuard() {
  if (pushed_) {
    active_nodes().pop_back();
  }
}

}  // namespace rfl::yaml
