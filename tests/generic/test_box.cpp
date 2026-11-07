#include <cassert>
#include <rfl.hpp>

#include "write_and_read.hpp"

namespace test_box {

struct DecisionTree {
  struct Leaf {
    using Tag = rfl::Literal<"Leaf">;
    double value;
  };

  struct Node {
    using Tag = rfl::Literal<"Node">;
    rfl::Rename<"criticalValue", double> critical_value;
    rfl::Box<DecisionTree> lesser;
    rfl::Box<DecisionTree> greater;
  };

  using LeafOrNode = rfl::TaggedUnion<"type", Leaf, Node>;

  rfl::Field<"leafOrNode", LeafOrNode> leaf_or_node;
};


TEST(generic, test_box) {
  auto leaf1 = DecisionTree::Leaf{.value = 3.0};

  auto leaf2 = DecisionTree::Leaf{.value = 5.0};

  auto node = DecisionTree::Node{
      .critical_value = 10.0,
      .lesser = rfl::make_box<DecisionTree>(DecisionTree{leaf1}),
      .greater = rfl::make_box<DecisionTree>(DecisionTree{leaf2})};

  const DecisionTree tree{.leaf_or_node = std::move(node)};

  write_and_read(tree);

}
}  // namespace test_box


namespace test_copyable_box
{
struct DecisionTree {
  struct Leaf {
    using Tag = rfl::Literal<"Leaf">;
    double value;
  };

  struct Node {
    using Tag = rfl::Literal<"Node">;
    rfl::Rename<"criticalValue", double> critical_value;
    rfl::CopyableBox<DecisionTree> lesser;
    rfl::CopyableBox<DecisionTree> greater;
  };

  using LeafOrNode = rfl::TaggedUnion<"type", Leaf, Node>;

  rfl::Field<"leafOrNode", LeafOrNode> leaf_or_node;
};
TEST(generic, copyable_box) {
  auto leaf1 = DecisionTree::Leaf{.value = 3.0};

  auto leaf2 = DecisionTree::Leaf{.value = 5.0};

  auto node = DecisionTree::Node{
      .critical_value = 10.0,
      .lesser = rfl::make_box<DecisionTree>(DecisionTree{leaf1}),
      .greater = rfl::make_box<DecisionTree>(DecisionTree{leaf2})};

  const DecisionTree tree1{.leaf_or_node = std::move(node)};

  write_and_read(tree1);

  const DecisionTree tree2 = tree1;

  auto lesser_leaf_tree2 = rfl::get<DecisionTree::Leaf>(rfl::get<DecisionTree::Node>(tree2.leaf_or_node.get().variant()).lesser.get()->leaf_or_node.get().variant());
  lesser_leaf_tree2.value = 1.0;


  auto lesser_leaf_tree1 = rfl::get<DecisionTree::Leaf>(rfl::get<DecisionTree::Node>(tree1.leaf_or_node.get().variant()).lesser.get()->leaf_or_node.get().variant());
  EXPECT_NE(lesser_leaf_tree1.value, lesser_leaf_tree2.value);

}

TEST(generic, copyable_box_copy_assignment) {
  const auto make_leaf = [](const double _value) {
    return rfl::make_copyable_box<DecisionTree>(
        DecisionTree{DecisionTree::Leaf{.value = _value}});
  };

  const auto value_of = [](const rfl::CopyableBox<DecisionTree>& _box) {
    return rfl::get<DecisionTree::Leaf>(_box->leaf_or_node.get().variant())
        .value;
  };

  const auto box1 = make_leaf(3.0);
  auto box2 = make_leaf(5.0);

  box2 = box1;

  EXPECT_EQ(value_of(box2), 3.0);
  EXPECT_NE(box1.get(), box2.get());

  const DecisionTree::Node node1{.critical_value = 10.0,
                                 .lesser = make_leaf(1.0),
                                 .greater = make_leaf(2.0)};
  auto node2 = DecisionTree::Node{.critical_value = 20.0,
                                  .lesser = make_leaf(4.0),
                                  .greater = make_leaf(5.0)};

  node2 = node1;

  EXPECT_EQ(node2.critical_value.get(), 10.0);
  EXPECT_EQ(value_of(node2.lesser), 1.0);
  EXPECT_EQ(value_of(node2.greater), 2.0);
  EXPECT_NE(node1.lesser.get(), node2.lesser.get());

  // The source may be part of the value it replaces.
  auto root = rfl::make_copyable_box<DecisionTree>(
      DecisionTree{.leaf_or_node = node1});
  root = rfl::get<DecisionTree::Node>(root->leaf_or_node.get().variant())
             .lesser;

  EXPECT_EQ(value_of(root), 1.0);
}
} // namespace test_copyable_box
