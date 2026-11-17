#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

// What does not change, and the limits of reading structs that contain a
// Validator without a valid default value.
namespace test_default_val_with_validator_limits {

using Path = std::vector<rfl::ErrorDetail::PathSegment>;

using Foods =
    rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>>;
using Positive = rfl::Validator<int, rfl::Minimum<1>>;

const std::string positive_error =
    "Value expected to be greater than or equal to 1, but got 0.";

std::string construction_error(const std::string& _type) {
  return "Could not default-construct "
         "test_default_val_with_validator_limits::" +
         _type +
         ", which is required because it contains rfl::DefaultVal. Give every "
         "field a valid default value. Original error: " +
         positive_error;
}

struct Person {
  rfl::DefaultVal<std::string> name = "Homer Simpson";
  Foods favorite_foods;
};

// A default member initializer that catches the exception of a Validator
// still sees it, as long as the struct can be default-constructed. If it
// cannot, every Validator that is default-constructed while the struct is
// constructed again keeps its invalid value, wherever it is, so the fallback
// is not used. Whether the Validator is returned through a temporary does not
// matter.
inline Positive port_or_fallback() {
  try {
    return Positive();
  } catch (std::exception&) {
    return Positive(8080);
  }
}

// Deferring does not touch the copy and move operations of Validator.
static_assert(std::is_trivially_copyable_v<Positive>);

struct Fallback {
  rfl::DefaultVal<std::string> host = "localhost";
  Positive port = port_or_fallback();
};

struct FallbackAndInvalid {
  Positive port = port_or_fallback();
  Positive retries;
};

TEST(json, test_default_val_with_validator_limits_fallback) {
  const auto res = rfl::json::read<Fallback, rfl::DefaultIfMissing>(R"({})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(rfl::json::write(*res), R"({"host":"localhost","port":8080})");

  const auto res2 = rfl::json::read<FallbackAndInvalid, rfl::DefaultIfMissing>(
      R"({"retries": 2})");
  ASSERT_FALSE(res2);
  EXPECT_EQ(std::string(res2.error().what()),
            "Field named 'port' not found, and its default value is "
            "invalid: " +
                positive_error);

  // If the input contains the field, it does not matter.
  const auto res3 = rfl::json::read<FallbackAndInvalid, rfl::DefaultIfMissing>(
      R"({"port": 443, "retries": 2})");
  ASSERT_TRUE(res3) << res3.error().what();
  EXPECT_EQ(rfl::json::write(*res3), R"({"port":443,"retries":2})");
}

// A read inside a default member initializer has its own invalid default
// values deferred, not those of the struct around it.
struct WithNestedRead {
  Person person =
      rfl::json::read<Person>(R"({"favorite_foods": ["beer"]})").value();
  rfl::DefaultVal<Positive> retries;
};

TEST(json, test_default_val_with_validator_limits_nested_read) {
  const auto res = rfl::json::read<WithNestedRead, rfl::DefaultIfMissing>(
      R"({"retries": 2})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->person.favorite_foods->at(0), "beer");
  EXPECT_EQ(res->retries.value().value(), 2);
}

// Validators that are default-constructed on other threads still throw.
TEST(json, test_default_val_with_validator_limits_threads) {
  std::atomic<bool> done = false;
  std::atomic<int> constructed = 0;
  auto other = std::thread([&] {
    while (!done) {
      try {
        Foods foods;
        ++constructed;
      } catch (std::exception&) {
      }
    }
  });
  auto readers = std::vector<std::thread>();
  std::atomic<int> failed_reads = 0;
  for (int i = 0; i < 2; ++i) {
    readers.emplace_back([&] {
      for (int j = 0; j < 2000; ++j) {
        if (!rfl::json::read<Person>(R"({"favorite_foods": ["donuts"]})")) {
          ++failed_reads;
        }
      }
    });
  }
  for (auto& reader : readers) {
    reader.join();
  }
  done = true;
  other.join();
  EXPECT_EQ(failed_reads, 0);
  EXPECT_EQ(constructed, 0);
}

// The regressions found in the review of an earlier attempt to fix upstream
// issue #700.
struct WithMap {
  rfl::DefaultVal<int> x;
  std::map<Positive, std::string> m;
};

struct NoDefaultConstructor {
  NoDefaultConstructor(int _v) : v_(_v) {}
  using ReflectionType = int;
  int reflection() const { return v_; }
  int v_;
};

struct NotDefaultConstructible {
  NoDefaultConstructor n;
  Positive p;
};

static_assert(!std::is_default_constructible_v<NotDefaultConstructible>);

struct ConstValidator {
  const rfl::Validator<unsigned, rfl::Minimum<1>> age;
  rfl::DefaultVal<std::string> name = "Maggie";
};

TEST(json, test_default_val_with_validator_limits_earlier_regressions) {
  EXPECT_FALSE(rfl::json::read<WithMap>(R"({"m": {"0": "zero"}})"));
  const auto map = rfl::json::read<WithMap>(R"({"m": {"1": "one"}})");
  ASSERT_TRUE(map) << map.error().what();
  EXPECT_EQ(map->m.begin()->first.value(), 1);

  const auto ndc =
      rfl::json::read<NotDefaultConstructible>(R"({"n": 3, "p": 4})");
  ASSERT_TRUE(ndc) << ndc.error().what();
  EXPECT_EQ(ndc->p.value(), 4);

  const auto con = rfl::json::read<ConstValidator>(R"({"age": 8})");
  ASSERT_TRUE(con) << con.error().what();
  EXPECT_EQ(rfl::json::write(*con), R"({"age":8,"name":"Maggie"})");
}

// A class that is not reflectable, kept out of reading by rfl::Skip. Reading
// cannot look into it, so with a deferred Validator in the struct, the
// skipped field cannot be checked and reading fails, as it did before.
class Cache {
 public:
  int hits() const { return hits_; }

 private:
  int hits_ = 0;
  std::string last_;
};

struct WithCache {
  rfl::DefaultVal<int> port = 8080;
  rfl::Skip<Cache> cache;
  Positive retries;
};

struct Schema {
  rfl::DefaultVal<bool> nullable = false;
  std::optional<std::vector<Schema>> all_of;
  std::optional<std::map<std::string, Schema>> properties;
  std::unique_ptr<Schema> items;
};

TEST(json, test_default_val_with_validator_limits_other_types) {
  const auto cache = rfl::json::read<WithCache>(R"({"retries": 1})");
  ASSERT_FALSE(cache);
  EXPECT_EQ(cache.error().what(),
            "Field named 'cache' not found, and its default value could not be "
            "checked: test_default_val_with_validator_limits::Cache");

  const auto schema = rfl::json::read<Schema>(
      R"({"properties": {"a": {"nullable": true}}, "items": {}})");
  ASSERT_TRUE(schema) << schema.error().what();
  EXPECT_TRUE(schema->properties->at("a").nullable.value());
  EXPECT_FALSE(schema->items->nullable.value());
}

// The JSON schema keeps the valid default values of a struct whose
// Validator has no valid default value. That Validator gets no default value.
struct WithPerson {
  int id;
  Person person;
};

const std::string person_schema =
    R"("test_default_val_with_validator_limits__Person":{"type":"object","properties":{"name":{"type":"string","default":"Homer Simpson"},"favorite_foods":{"type":"array","items":{"type":"string"},"minItems":1}},"required":["favorite_foods"]})";

TEST(json, test_default_val_with_validator_limits_to_schema) {
  EXPECT_EQ(
      rfl::json::to_schema<Person>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_default_val_with_validator_limits__Person","$defs":{)" +
          person_schema + "}}");
}

TEST(json, test_default_val_with_validator_limits_to_schema_nested) {
  EXPECT_EQ(
      rfl::json::to_schema<WithPerson>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_default_val_with_validator_limits__WithPerson","$defs":{)" +
          person_schema +
          R"(,"test_default_val_with_validator_limits__WithPerson":{"type":"object","properties":{"id":{"type":"integer"},"person":{"$ref":"#/$defs/test_default_val_with_validator_limits__Person"}},"required":["id","person"]}}})");
}

// Two Validators without a valid default value, one rfl::DefaultVal with a
// valid one, and one rfl::DefaultVal whose default value is invalid. That one
// gets no default value and is required, as reading requires it.
struct Server {
  rfl::DefaultVal<int> retries = 3;
  Positive port;
  rfl::Validator<std::string, rfl::Size<rfl::Minimum<1>>> host;
  rfl::DefaultVal<Positive> threads;
};

TEST(json, test_default_val_with_validator_limits_to_schema_several) {
  EXPECT_EQ(
      rfl::json::to_schema<Server>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_default_val_with_validator_limits__Server","$defs":{"test_default_val_with_validator_limits__Server":{"type":"object","properties":{"retries":{"type":"integer","default":3},"port":{"minimum":1,"type":"integer"},"host":{"type":"string","minLength":1},"threads":{"minimum":1,"type":"integer"}},"required":["port","host","threads"]}}})");

  const auto server =
      rfl::json::read<Server>(R"({"port":80,"host":"h","threads":2})");
  ASSERT_TRUE(server) << server.error().what();
  EXPECT_EQ(server.value().retries.value(), 3);
  EXPECT_FALSE(rfl::json::read<Server>(R"({"port":80,"host":"h"})"));
}

// [[no_unique_address]] can place a field in the tail padding of another.
struct InTailPadding {
  rfl::Validator<int64_t, rfl::Minimum<0>> a;
  int8_t c = 0;
};

struct SharesPadding {
  [[no_unique_address]] InTailPadding inner;
  rfl::DefaultVal<Positive> p;
};

TEST(json, test_default_val_with_validator_limits_no_unique_address) {
  const auto res =
      rfl::json::read<SharesPadding>(R"({"inner": {"a": 1, "c": 2}})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().details().at(0).path, Path({"p"}));

  const auto res2 =
      rfl::json::read<SharesPadding>(R"({"inner": {"a": 1, "c": 2}, "p": 3})");
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_EQ(res2->p.value().value(), 3);
}

// A field in the input replaces the default value as a whole. Should a field
// ever be read into its default value instead, the invalid default of p would
// survive here.
struct Inner {
  Positive p;
  rfl::DefaultVal<int> q;
};

struct Outer {
  rfl::DefaultVal<int> x;
  Inner inner;
};

TEST(json, test_default_val_with_validator_limits_found_fields_are_replaced) {
  const auto res =
      rfl::json::read<Outer, rfl::DefaultIfMissing>(R"({"inner": {"q": 1}})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().details().at(0).path, Path({"inner", "p"}));
}

// A Validator that is default-constructed elsewhere while the struct is
// constructed, such as a temporary or on the heap, keeps its invalid value as
// well. The fields that hold it must then be in the input.
struct OnTheHeap {
  rfl::DefaultVal<int> x;
  std::vector<Positive> v = std::vector<Positive>(1);
};

struct FromTemporary {
  rfl::DefaultVal<int> x;
  rfl::DefaultVal<Positive> p = Positive();
};

struct InRflTuple {
  rfl::DefaultVal<int> x;
  rfl::Tuple<Positive, int> t;
};

TEST(json, test_default_val_with_validator_limits_not_in_place) {
  const auto heap = rfl::json::read<OnTheHeap>(R"({"v": [2]})");
  ASSERT_TRUE(heap) << heap.error().what();
  EXPECT_EQ(heap->v.at(0).value(), 2);
  const auto heap_missing =
      rfl::json::read<OnTheHeap, rfl::DefaultIfMissing>(R"({"x": 1})");
  ASSERT_FALSE(heap_missing);
  EXPECT_EQ(heap_missing.error().what(),
            "Field named 'v' not found, and its default value is invalid: " +
                positive_error);

  const auto temp = rfl::json::read<FromTemporary>(R"({"p": 2})");
  ASSERT_TRUE(temp) << temp.error().what();
  EXPECT_EQ(temp->p.value().value(), 2);
  EXPECT_FALSE(rfl::json::read<FromTemporary>(R"({"x": 1})"));

  const auto tuple = rfl::json::read<InRflTuple>(R"({"t": [2, 3]})");
  ASSERT_TRUE(tuple) << tuple.error().what();
  EXPECT_EQ(rfl::json::write(*tuple), R"({"x":0,"t":[2,3]})");
  EXPECT_FALSE(rfl::json::read<InRflTuple>(R"({"x": 1})"));
}

// A field that is never read keeps its default value.
struct SkipsValidator {
  rfl::DefaultVal<int> x;
  rfl::Skip<Positive> s;
};

TEST(json, test_default_val_with_validator_limits_skip) {
  const auto res = rfl::json::read<SkipsValidator>(R"({"x": 1})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Field named 's' not found, and its default value is invalid: " +
                positive_error);

  const auto supplied = rfl::json::read<SkipsValidator>(R"({"x": 1, "s": 5})");
  ASSERT_FALSE(supplied);
}

// A copy of a Validator whose default value is invalid is invalid as well,
// so it must be in the input like the original, wherever it is.
struct Copies {
  Positive a;
  rfl::DefaultVal<Positive> b = a;
};

struct CopiesToHeap {
  rfl::DefaultVal<int> x;
  Positive a;
  std::vector<Positive> v = {a};
};

TEST(json, test_default_val_with_validator_limits_copies) {
  EXPECT_THROW((void)Copies{}, std::exception);
  const auto res = rfl::json::read<Copies>(R"({"a": 5})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Field named 'b' not found, and its default value is invalid: " +
                positive_error);

  const auto res2 = rfl::json::read<Copies>(R"({"a": 5, "b": 6})");
  ASSERT_TRUE(res2) << res2.error().what();
  EXPECT_EQ(res2->b.value().value(), 6);

  const auto heap = rfl::json::read<CopiesToHeap>(R"({"a": 5, "v": [6]})");
  ASSERT_TRUE(heap) << heap.error().what();
  EXPECT_EQ(heap->v.at(0).value(), 6);
  const auto heap_missing =
      rfl::json::read<CopiesToHeap, rfl::DefaultIfMissing>(R"({"a": 5})");
  ASSERT_FALSE(heap_missing);
  EXPECT_EQ(heap_missing.error().what(),
            "Field named 'v' not found, and its default value is invalid: " +
                positive_error);
}

// An invalid default value that a default member initializer has used up,
// such as a temporary, is not in the struct any more, so reading cannot check
// it and fails as before.
struct FromTemporaryValue {
  rfl::DefaultVal<int> x;
  int port = Positive().value();
};

// The same holds for an invalid default value inside a class that reading
// cannot look into.
class Opaque {
 public:
  using ReflectionType = int;
  Opaque() = default;
  explicit Opaque(int _v) : p_(_v) {}
  int get() const { return p_.value(); }

 private:
  Positive p_;
};

struct WithOpaque {
  rfl::DefaultVal<int> x;
  rfl::DefaultVal<Opaque> o;
};

// A missing field of such a class cannot be checked either, even if it holds
// no Validator.
class Port {
 public:
  using ReflectionType = int;
  Port() = default;
  explicit Port(int _v) : v_(_v) {}
  int get() const { return v_; }

 private:
  int v_ = 80;
};

struct WithPort {
  Positive retries;
  rfl::DefaultVal<Port> port;
};

TEST(json, test_default_val_with_validator_limits_unchecked) {
  EXPECT_THROW((void)FromTemporaryValue{}, std::exception);
  const auto temp = rfl::json::read<FromTemporaryValue, rfl::DefaultIfMissing>(
      R"({"x": 1, "port": 2})");
  ASSERT_FALSE(temp);
  EXPECT_EQ(temp.error().what(), construction_error("FromTemporaryValue"));

  EXPECT_THROW((void)WithOpaque{}, std::exception);
  const auto opaque = rfl::json::read<WithOpaque>(R"({"x": 1, "o": 3})");
  ASSERT_FALSE(opaque);
  EXPECT_EQ(opaque.error().what(), construction_error("WithOpaque"));

  const auto port = rfl::json::read<WithPort>(R"({"retries": 1})");
  ASSERT_FALSE(port);
  EXPECT_EQ(port.error().what(),
            "Field named 'port' not found, and its default value could not be "
            "checked: test_default_val_with_validator_limits::Port");
  const auto port_present =
      rfl::json::read<WithPort>(R"({"retries": 1, "port": 8080})");
  ASSERT_TRUE(port_present) << port_present.error().what();
  EXPECT_EQ(port_present->port.value().get(), 8080);
}

// The gaps: a default member initializer that reads a Validator gets its
// invalid value, and a copy of an invalid Validator makes up for a used-up
// temporary, which then goes unnoticed.
struct ReadsValidator {
  Positive a;
  int b = a.value() * 2;
};

struct CopyAndTemporary {
  Positive a;
  rfl::DefaultVal<Positive> b = a;
  int port = Positive().value();
};

TEST(json, test_default_val_with_validator_limits_gaps) {
  const auto reads =
      rfl::json::read<ReadsValidator, rfl::DefaultIfMissing>(R"({"a": 5})");
  ASSERT_TRUE(reads) << reads.error().what();
  EXPECT_EQ(reads->b, 0);

  const auto copy = rfl::json::read<CopyAndTemporary, rfl::DefaultIfMissing>(
      R"({"a": 5, "b": 6})");
  ASSERT_TRUE(copy) << copy.error().what();
  EXPECT_EQ(copy->port, 0);
}

struct NonCopyableSkip {
  rfl::DefaultVal<int> x = 1;
  rfl::Skip<std::unique_ptr<int>> handle;
  rfl::Skip<Positive*> ptr = nullptr;
  rfl::Skip<void*> ctx = nullptr;
};

struct WithRawPositivePtr {
  rfl::DefaultVal<int> x = 1;
  Positive* ptr = nullptr;
};

struct NonMovableSchema {
  rfl::DefaultVal<int> x = 1;
  std::atomic<int> counter{0};
};

struct NestedInner {
  Positive x;
};

struct NonZeroNestedInner {
  static rfl::Result<NestedInner> validate(NestedInner _i) {
    return _i.x.value() > 0 ? rfl::Result<NestedInner>(_i)
                            : rfl::error("bad inner");
  }
};

struct NestedValidatorStruct {
  rfl::DefaultVal<int> id = 0;
  rfl::Validator<NestedInner, NonZeroNestedInner> v;
};

struct WithExtraPort {
  rfl::DefaultVal<int> x = 1;
  Positive p;
  rfl::ExtraFields<Port> extra;
};

struct CycleNode {
  std::shared_ptr<CycleNode> next;
};

inline std::shared_ptr<CycleNode> make_cycle() {
  auto a = std::make_shared<CycleNode>();
  auto b = std::make_shared<CycleNode>();
  a->next = b;
  b->next = a;
  return a;
}

struct SharedCycleNode {
  rfl::DefaultVal<int> id = 0;
  Positive p;
  rfl::Skip<std::shared_ptr<CycleNode>> cycle = make_cycle();
};

struct SharedAlias {
  rfl::DefaultVal<int> x = 0;
  std::shared_ptr<Positive> a = std::make_shared<Positive>();
  std::shared_ptr<Positive> b = a;
  int port = Positive().value();
};

TEST(json, test_default_val_with_validator_limits_regressions) {
  // Non-copyable rfl::Skip, raw Positive*, and nullptr void* must compile.
  const auto ncs = rfl::json::read<NonCopyableSkip>(R"({})");
  ASSERT_TRUE(ncs) << ncs.error().what();
  EXPECT_EQ(ncs->x.value(), 1);

  const auto raw_ptr =
      rfl::json::read<WithRawPositivePtr, rfl::AllowRawPtrs>(R"({"ptr": null})");
  ASSERT_TRUE(raw_ptr) << raw_ptr.error().what();
  EXPECT_EQ(raw_ptr->ptr, nullptr);

  // Non-movable struct with DefaultVal must still generate a schema.
  const auto nms = rfl::json::to_schema<NonMovableSchema>();
  EXPECT_NE(nms.find(R"("default":1)"), std::string::npos) << nms;

  // Nested Validator must not double-count inner deferred Validators.
  const auto nested =
      rfl::json::read<NestedValidatorStruct>(R"({"v": {"x": 5}})");
  ASSERT_TRUE(nested) << nested.error().what();
  EXPECT_EQ(nested->v.value().x.value(), 5);
  EXPECT_FALSE(
      (rfl::json::read<NestedValidatorStruct, rfl::DefaultIfMissing>(R"({})")));

  // Input-parsed ExtraFields elements must not be rejected as unchecked defaults.
  const auto extra = rfl::json::read<WithExtraPort>(R"({"p": 1, "k": 80})");
  ASSERT_TRUE(extra) << extra.error().what();
  EXPECT_EQ(extra->extra.at("k").get(), 80);

  // Cyclic shared_ptr graph must not stack-overflow.
  const auto cyc = rfl::json::read<SharedCycleNode>(R"({"p": 7})");
  ASSERT_TRUE(cyc) << cyc.error().what();
  EXPECT_EQ(cyc->p.value(), 7);
  // Break cycle before destruction to avoid leaking under ASan/LSan.
  cyc->cycle.value()->next.reset();

  // Two shared_ptrs to one invalid Validator must not double-count to mask a
  // discarded temporary.
  const auto alias =
      rfl::json::read<SharedAlias>(R"({"a": 1, "b": 2, "port": 3})");
  ASSERT_FALSE(alias);

  // When !holds_all_deferred, to_schema must still mark DefaultVal fields
  // required (no DefaultVal wrapper left behind).
  const auto temp_schema = rfl::json::to_schema<FromTemporaryValue>();
  EXPECT_NE(temp_schema.find(R"("required":["x","port"])"), std::string::npos)
      << temp_schema;
}

}  // namespace test_default_val_with_validator_limits
