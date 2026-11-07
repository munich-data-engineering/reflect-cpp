#ifndef RFL_PARSING_VIEWREADERWITHSTRIPPEDFIELDNAMES_HPP_
#define RFL_PARSING_VIEWREADERWITHSTRIPPEDFIELDNAMES_HPP_

#include <array>
#include <optional>
#include <sstream>
#include <type_traits>
#include <utility>

#include "../Result.hpp"
#include "../Tuple.hpp"
#include "../internal/is_array.hpp"
#include "FieldErrors.hpp"
#include "Parser_base.hpp"
#include "wrap_error.hpp"

namespace rfl::parsing {

template <class R, class W, class _ViewType, class ProcessorsType>
class ViewReaderWithStrippedFieldNames {
 public:
  using InputVarType = typename R::InputVarType;
  using ViewType = _ViewType;

  static constexpr size_t size_ = ViewType::size();

  /**
   * @brief Constructor.
   *
   * @param _r The reader to use.
   * @param _view The view to read into.
   * @param _found A boolean array indicating which fields have been found.
   * @param _set A boolean array indicating which fields have been successfully
   * set.
   * @param _errors The field error collector.
   */
  ViewReaderWithStrippedFieldNames(const R* _r, ViewType* _view,
                                   std::array<bool, size_>* _found,
                                   std::array<bool, size_>* _set,
                                   FieldErrors<ProcessorsType>* _errors)
      : i_(0),
        r_(_r),
        view_(_view),
        found_(_found),
        set_(_set),
        errors_(_errors) {}

  ~ViewReaderWithStrippedFieldNames() = default;

  /**
   * @brief Reads a single variable from the input.
   *
   * @param _var The input variable to read from.
   * @return An optional error.
   */
  std::optional<Error> read(const InputVarType& _var) const {
    if (should_stop_on_field_error<ProcessorsType>(*errors_)) return std::nullopt;
    if (i_ == size_) {
      std::stringstream stream;
      stream << "Expected a maximum of " << std::to_string(size_)
             << " fields, but got at least one more.";
      return Error(stream.str());
    }
    assign_to_field_i(_var, std::make_integer_sequence<int, size_>());
    ++i_;
    return std::nullopt;
  }

  /**
   * @brief Returns the size of the view.
   *
   * @return The size of the view.
   */
  static constexpr size_t size() { return size_; }

 private:
  template <int i>
  static void assign_if_field_is_field_i(const R& _r, const auto& _var,
                                         auto* _view, auto* _errors,
                                         auto* _found, auto* _set, int _i) {
    using FieldType = tuple_element_t<i, typename ViewType::Fields>;
    using OriginalType = typename FieldType::Type;
    using T =
        std::remove_cvref_t<std::remove_pointer_t<typename FieldType::Type>>;
    constexpr auto name = FieldType::name();
    if (_i == i) {
      std::get<i>(*_found) = true;
      auto res = Parser<R, W, T, ProcessorsType>::read(_r, _var);
      if (!res) {
        _errors->emplace_back(wrap_field_error(name, std::move(res).error()));
        return;
      }
      if constexpr (std::is_pointer_v<OriginalType>) {
        move_to(rfl::get<i>(*_view), &(*res));
      } else {
        rfl::get<i>(*_view) = std::move(*res);
      }
      std::get<i>(*_set) = true;
    }
  }

  template <int... is>
  void assign_to_field_i(const InputVarType& _var,
                         std::integer_sequence<int, is...>) const {
    using ReadField =
        void (*)(const ViewReaderWithStrippedFieldNames*, const InputVarType&);
    static constexpr std::array<ReadField, sizeof...(is)> readers{
        +[](const ViewReaderWithStrippedFieldNames* self,
            const InputVarType& var) {
          assign_if_field_is_field_i<is>(*self->r_, var, self->view_,
                                         self->errors_, self->found_,
                                         self->set_, is);
        }...};
    readers[i_](this, _var);
  }

  // TODO: Unnecessary code duplication.
  template <class Target, class Source>
  static void move_to(Target* _t, Source* _s) {
    if constexpr (std::is_const_v<Target>) {
      return move_to(const_cast<std::remove_const_t<Target>*>(_t), _s);
    } else if constexpr (!rfl::internal::is_array_v<Source> &&
                         !std::is_array_v<Target>) {
      ::new (_t) Target(std::move(*_s));
    } else if constexpr (rfl::internal::is_array_v<Source>) {
      static_assert(std::is_array_v<Target>,
                    "Expected target to be a c-array.");
      for (size_t i = 0; i < _s->arr_.size(); ++i) {
        move_to(&((*_t)[i]), &(_s->arr_[i]));
      }
    } else {
      for (size_t i = 0; i < _s->size(); ++i) {
        move_to(&((*_t)[i]), &((*_s)[i]));
      }
    }
  }

 private:
  /// Indicates the current field.
  mutable int i_;

  /// The underlying reader.
  const R* r_;

  /// The underlying view.
  ViewType* view_;

  /// Indicates that a certain field has been found.
  std::array<bool, size_>* found_;

  /// Indicates that a certain field has been successfully set - necessary,
  /// because we have to trigger the destructors manually.
  std::array<bool, size_>* set_;

  /// Stores field errors according to the selected processor policy.
  FieldErrors<ProcessorsType>* errors_;
};

}  // namespace rfl::parsing

#endif
