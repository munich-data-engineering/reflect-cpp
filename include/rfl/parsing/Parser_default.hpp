#ifndef RFL_PARSING_PARSER_DEFAULT_HPP_
#define RFL_PARSING_PARSER_DEFAULT_HPP_

#include <map>
#include <type_traits>

#include "../Result.hpp"
#include "../always_false.hpp"
#include "../internal/HasFromReflectionMethod.hpp"
#include "../internal/always_inline.hpp"
#include "../internal/default_if_missing_v.hpp"
#include "../internal/has_default_val_v.hpp"
#include "../internal/has_reflection_method_v.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/has_reflector.hpp"
#include "../internal/is_basic_type.hpp"
#include "../internal/is_deprecated.hpp"
#include "../internal/is_description.hpp"
#include "../internal/is_literal.hpp"
#include "../internal/is_validator.hpp"
#include "../internal/processed_t.hpp"
#include "../internal/ptr_cast.hpp"
#include "../internal/to_ptr_named_tuple.hpp"
#include "../to_view.hpp"
#include "AreReaderAndWriter.hpp"
#include "MapParser.hpp"
#include "Parent.hpp"
#include "ParserArray.hpp"
#include "ParserAtomic.hpp"
#include "ParserAtomicFlag.hpp"
#include "ParserBasicType.hpp"
#include "ParserBox.hpp"
#include "ParserBytestring.hpp"
#include "ParserCArray.hpp"
#include "ParserCommented.hpp"
#include "ParserDefaultVal.hpp"
#include "ParserDuration.hpp"
#include "ParserEnum.hpp"
#include "ParserExpected.hpp"
#include "ParserFilepath.hpp"
#include "ParserMonostate.hpp"
#include "ParserOptional.hpp"
#include "ParserPair.hpp"
#include "ParserPositional.hpp"
#include "ParserPtr.hpp"
#include "ParserRef.hpp"
#include "ParserReferenceWrapper.hpp"
#include "ParserRename.hpp"
#include "ParserResult.hpp"
#include "ParserRflArray.hpp"
#include "ParserRflTuple.hpp"
#include "ParserRflVariant.hpp"
#include "ParserSharedPtr.hpp"
#include "ParserShort.hpp"
#include "ParserSkip.hpp"
#include "ParserSpan.hpp"
#include "ParserStringView.hpp"
#include "ParserTaggedUnion.hpp"
#include "ParserTimePoint.hpp"
#include "ParserTuple.hpp"
#include "ParserUniquePtr.hpp"
#include "ParserVariant.hpp"
#include "ParserVectorstring.hpp"
#include "ParserWString.hpp"
#include "Parser_base.hpp"
#include "VectorParser.hpp"
#include "call_destructors_where_necessary.hpp"
#include "is_string_map.hpp"
#include "make_type_name.hpp"
#include "schema/Type.hpp"
#include "schemaful/IsSchemafulReader.hpp"
#include "schemaful/IsSchemafulWriter.hpp"

namespace rfl::parsing {

/// Default case - anything that cannot be explicitly matched.
template <class R, class W, class T, class ProcessorsType>
  requires AreReaderAndWriter<R, W, T>
struct Parser {
 public:
  using InputVarType = typename R::InputVarType;

  using ParentType = Parent<W>;

  /**
   * @brief Reads a value from the input.
   *
   * @param _r The reader to use.
   * @param _var The input variable to read from.
   * @return A Result containing the parsed value or an error.
   */
  static RFL_ALWAYS_INLINE auto read(const R& _r, const InputVarType& _var) {
    if constexpr (internal::is_basic_type_v<T>) {
      return ParserBasicType<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_bytestring_v<T>) {
      return ParserBytestring<R, W, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_vectorstring_v<T>) {
      return ParserVectorstring<R, W, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_string_map_v<T>) {
      return MapParser<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_vector_like_v<T>) {
      return VectorParser<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_shared_ptr_v<T>) {
      return ParserSharedPtr<R, W, typename T::element_type,
                             ProcessorsType>::read(_r, _var);

    } else if constexpr (is_unique_ptr_v<T>) {
      return ParserUniquePtr<R, W, typename T::element_type,
                             ProcessorsType>::read(_r, _var);

    } else if constexpr (is_optional_v<T>) {
      return ParserOptional<R, W, typename std::remove_cvref_t<T>::value_type,
                            ProcessorsType>::read(_r, _var);

    } else if constexpr (is_box_v<T>) {
      using IsBox = is_box<std::remove_cvref_t<T>>;
      return ParserBox<R, W, typename IsBox::element_type, IsBox::copyability,
                       ProcessorsType>::read(_r, _var);

    } else if constexpr (is_ref_v<T>) {
      using IsRef = is_ref<std::remove_cvref_t<T>>;
      return ParserRef<R, W, typename IsRef::element_type,
                       ProcessorsType>::read(_r, _var);

    } else if constexpr (is_variant_v<T>) {
      return ParserVariant<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_rfl_variant_v<T>) {
      return ParserRflVariant<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_tagged_union_v<T>) {
      return ParserTaggedUnion<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_rename_v<T>) {
      return ParserRename<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_result_v<T>) {
      return ParserResult<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_expected_v<T>) {
      return ParserExpected<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_duration_v<T>) {
      using U = std::remove_cvref_t<T>;
      return ParserDuration<R, W, typename U::rep, typename U::period,
                            ProcessorsType>::read(_r, _var);

    } else if constexpr (is_filepath_v<T>) {
      return ParserFilepath<R, W, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_monostate_v<T>) {
      return ParserMonostate<R, W, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_skip_v<T>) {
      using U = std::remove_cvref_t<T>;
      return ParserSkip<R, W, typename U::Type, U::skip_serialization_,
                        U::skip_deserialization_, ProcessorsType>::read(_r,
                                                                        _var);

    } else if constexpr (is_span_v<T>) {
      return ParserSpan<R, W, typename std::remove_cvref_t<T>::element_type,
                        ProcessorsType>::read(_r, _var);

    } else if constexpr (is_time_point_v<T>) {
      return ParserTimePoint<R, W, typename std::remove_cvref_t<T>::duration,
                             ProcessorsType>::read(_r, _var);

    } else if constexpr (is_default_val_v<T>) {
      return ParserDefaultVal<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_short_v<T>) {
      return ParserShort<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_positional_v<T>) {
      return ParserPositional<R, W, typename std::remove_cvref_t<T>::Type,
                              ProcessorsType>::read(_r, _var);

    } else if constexpr (is_reference_wrapper_v<T>) {
      return ParserReferenceWrapper<R, W, typename std::remove_cvref_t<T>::type,
                                    ProcessorsType>::read(_r, _var);

    } else if constexpr (is_array_v<T>) {
      using IsArray = is_array<T>;
      return ParserArray<R, W, typename IsArray::element_type, IsArray::size,
                         ProcessorsType>::read(_r, _var);

    } else if constexpr (is_rfl_array_v<T>) {
      return ParserRflArray<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_tuple_v<T>) {
      return ParserTuple<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_rfl_tuple_v<T>) {
      return ParserRflTuple<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_pair_v<T>) {
      return ParserPair<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (std::is_enum_v<T>) {
      return ParserEnum<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_wstring_v<T>) {
      return ParserWString<R, W, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_string_view_v<T>) {
      return ParserStringView<R, W, ProcessorsType>::read(_r, _var);

    } else if constexpr (is_commented_v<T>) {
      return ParserCommented<R, W, T, ProcessorsType>::read(_r, _var);

    } else if constexpr (std::is_pointer_v<T>) {
      return ParserPtr<R, W, std::remove_cvref_t<std::remove_pointer_t<T>>,
                       ProcessorsType>::read(_r, _var);

    } else if constexpr (is_c_array_v<T>) {
      using IsCArray = is_c_array<T>;
      return ParserCArray<R, W, typename IsCArray::element_type, IsCArray::size,
                          ProcessorsType>::read(_r, _var);

    } else if constexpr (internal::has_read_reflector<T>) {
      const auto wrap_in_t = [](auto&& _named_tuple) -> Result<T> {
        try {
          using NT = decltype(_named_tuple);
          return Reflector<T>::to(std::forward<NT>(_named_tuple));
        } catch (std::exception& e) {
          return error(e.what());
        }
      };
      return Parser<R, W, typename Reflector<T>::ReflType,
                    ProcessorsType>::read(_r, _var)
          .and_then(wrap_in_t);

    } else if constexpr (schemaful::IsSchemafulReader<R> &&
                         internal::is_literal_v<T>) {
      return _r.template to_basic_type<T>(_var);

    } else if constexpr (R::template has_custom_constructor<T>) {
      return _r.template use_custom_constructor<T>(_var);

    } else {
      if constexpr (internal::has_reflection_type_v<T>) {
        using ReflectionType = std::remove_cvref_t<typename T::ReflectionType>;
        const auto wrap_in_t = [](auto&& _named_tuple) -> Result<T> {
          try {
            using NT = decltype(_named_tuple);
            if constexpr (internal::HasFromReflectionMethod<T>) {
              return T::from_reflection(std::forward<NT>(_named_tuple));
            } else {
              return T{std::forward<NT>(_named_tuple)};
            }
          } catch (std::exception& e) {
            return error(e.what());
          }
        };
        return Parser<R, W, ReflectionType, ProcessorsType>::read(_r, _var)
            .and_then(wrap_in_t);

      } else if constexpr (internal::default_if_missing_v<ProcessorsType> ||
                           internal::has_default_val_v<T>) {
        return read_struct_with_default(_r, _var);

      } else {
        return read_struct(_r, _var);
      }
    }
  }

  /**
   * @brief Writes a value to the output.
   *
   * @tparam P The type of the parent.
   * @param _w The writer to use.
   * @param _var The value to write.
   * @param _parent The parent object.
   */
  template <class P>
  static void write(const W& _w, const T& _var, const P& _parent) {
    if constexpr (internal::is_basic_type_v<T>) {
      ParserBasicType<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_bytestring_v<T>) {
      ParserBytestring<R, W, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_vectorstring_v<T>) {
      ParserVectorstring<R, W, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_string_map_v<T>) {
      return MapParser<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_vector_like_v<T>) {
      return VectorParser<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_shared_ptr_v<T>) {
      ParserSharedPtr<R, W, typename T::element_type, ProcessorsType>::write(
          _w, _var, _parent);

    } else if constexpr (is_unique_ptr_v<T>) {
      ParserUniquePtr<R, W, typename T::element_type, ProcessorsType>::write(
          _w, _var, _parent);

    } else if constexpr (is_optional_v<T>) {
      ParserOptional<R, W, typename std::remove_cvref_t<T>::value_type,
                     ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_box_v<T>) {
      using IsBox = is_box<std::remove_cvref_t<T>>;
      ParserBox<R, W, typename IsBox::element_type, IsBox::copyability,
                ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_ref_v<T>) {
      using IsRef = is_ref<std::remove_cvref_t<T>>;
      ParserRef<R, W, typename IsRef::element_type, ProcessorsType>::write(
          _w, _var, _parent);

    } else if constexpr (is_result_v<T>) {
      ParserResult<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_expected_v<T>) {
      ParserExpected<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_variant_v<T>) {
      ParserVariant<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_rfl_variant_v<T>) {
      ParserRflVariant<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_tagged_union_v<T>) {
      ParserTaggedUnion<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_rename_v<T>) {
      ParserRename<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_duration_v<T>) {
      using U = std::remove_cvref_t<T>;
      ParserDuration<R, W, typename U::rep, typename U::period,
                     ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_filepath_v<T>) {
      ParserFilepath<R, W, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_monostate_v<T>) {
      ParserMonostate<R, W, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_skip_v<T>) {
      using U = std::remove_cvref_t<T>;
      ParserSkip<R, W, typename U::Type, U::skip_serialization_,
                 U::skip_deserialization_, ProcessorsType>::write(_w, _var,
                                                                  _parent);

    } else if constexpr (is_span_v<T>) {
      ParserSpan<R, W, typename std::remove_cvref_t<T>::element_type,
                 ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_time_point_v<T>) {
      ParserTimePoint<R, W, typename std::remove_cvref_t<T>::duration,
                      ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_default_val_v<T>) {
      ParserDefaultVal<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_short_v<T>) {
      ParserShort<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_positional_v<T>) {
      ParserPositional<R, W, typename std::remove_cvref_t<T>::Type,
                       ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_reference_wrapper_v<T>) {
      ParserReferenceWrapper<R, W, typename std::remove_cvref_t<T>::type,
                             ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_array_v<T>) {
      using IsArray = is_array<T>;
      ParserArray<R, W, typename IsArray::element_type, IsArray::size,
                  ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_rfl_array_v<T>) {
      ParserRflArray<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_tuple_v<T>) {
      ParserTuple<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_rfl_tuple_v<T>) {
      ParserRflTuple<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_pair_v<T>) {
      ParserPair<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (std::is_enum_v<T>) {
      ParserEnum<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_wstring_v<T>) {
      ParserWString<R, W, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_string_view_v<T>) {
      ParserStringView<R, W, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_commented_v<T>) {
      ParserCommented<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (std::is_pointer_v<T>) {
      ParserPtr<R, W, std::remove_cvref_t<std::remove_pointer_t<T>>,
                ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_c_array_v<T>) {
      using IsCArray = is_c_array<T>;
      ParserCArray<R, W, typename IsCArray::element_type, IsCArray::size,
                   ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_atomic_v<T>) {
      ParserAtomic<R, W, T, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (is_atomic_flag_v<T>) {
      ParserAtomicFlag<R, W, ProcessorsType>::write(_w, _var, _parent);

    } else if constexpr (internal::has_write_reflector<T>) {
      Parser<R, W, typename Reflector<T>::ReflType, ProcessorsType>::write(
          _w, Reflector<T>::from(_var), _parent);

    } else if constexpr (schemaful::IsSchemafulWriter<W> &&
                         internal::is_literal_v<T>) {
      ParentType::add_value(_w, _var, _parent);

    } else if constexpr (internal::has_reflection_type_v<T>) {
      using ReflectionType = std::remove_cvref_t<typename T::ReflectionType>;
      if constexpr (internal::has_reflection_method_v<T>) {
        Parser<R, W, ReflectionType, ProcessorsType>::write(
            _w, _var.reflection(), _parent);
      } else {
        const auto& [r] = _var;
        Parser<R, W, ReflectionType, ProcessorsType>::write(_w, r, _parent);
      }

    } else {
      const auto ptr_named_tuple = ProcessorsType::template process<T>(
          internal::to_ptr_named_tuple(_var));
      using PtrNamedTupleType = std::remove_cvref_t<decltype(ptr_named_tuple)>;
      Parser<R, W, PtrNamedTupleType, ProcessorsType>::write(
          _w, ptr_named_tuple, _parent);
    }
  }

  /**
   * @brief Generates the schema for the type.
   *
   * @param _definitions The map of definitions to add to.
   * @return The schema type.
   */
  static schema::Type to_schema(
      std::map<std::string, schema::Type>* _definitions) {
    using U = std::remove_cvref_t<T>;
    using Type = schema::Type;

    if constexpr (internal::is_basic_type_v<U>) {
      return ParserBasicType<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_bytestring_v<U>) {
      return ParserBytestring<R, W, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_vectorstring_v<U>) {
      return ParserVectorstring<R, W, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_string_map_v<U>) {
      return MapParser<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_vector_like_v<U>) {
      return VectorParser<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_shared_ptr_v<U>) {
      return ParserSharedPtr<R, W, typename U::element_type,
                             ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_unique_ptr_v<U>) {
      return ParserUniquePtr<R, W, typename U::element_type,
                             ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_optional_v<U>) {
      return ParserOptional<R, W, typename U::value_type,
                            ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_box_v<U>) {
      using IsBox = is_box<U>;
      return ParserBox<R, W, typename IsBox::element_type, IsBox::copyability,
                       ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_ref_v<U>) {
      using IsRef = is_ref<U>;
      return ParserRef<R, W, typename IsRef::element_type,
                       ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_variant_v<U>) {
      return ParserVariant<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_rfl_variant_v<U>) {
      return ParserRflVariant<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_tagged_union_v<U>) {
      return ParserTaggedUnion<R, W, U, ProcessorsType>::to_schema(
          _definitions);

    } else if constexpr (is_rename_v<U>) {
      return ParserRename<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_result_v<U>) {
      return ParserResult<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_expected_v<U>) {
      return ParserExpected<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_duration_v<U>) {
      return ParserDuration<R, W, typename U::rep, typename U::period,
                            ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_filepath_v<U>) {
      return ParserFilepath<R, W, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_monostate_v<U>) {
      return ParserMonostate<R, W, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_skip_v<U>) {
      return ParserSkip<R, W, typename U::Type, U::skip_serialization_,
                        U::skip_deserialization_,
                        ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_span_v<U>) {
      return ParserSpan<R, W, typename U::element_type,
                        ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_time_point_v<U>) {
      return ParserTimePoint<R, W, typename U::duration,
                             ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_default_val_v<U>) {
      return ParserDefaultVal<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_short_v<U>) {
      return ParserShort<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_positional_v<U>) {
      return ParserPositional<R, W, typename U::Type,
                              ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_reference_wrapper_v<U>) {
      return ParserReferenceWrapper<R, W, typename U::type,
                                    ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_array_v<U>) {
      using IsArray = is_array<U>;
      return ParserArray<R, W, typename IsArray::element_type, IsArray::size,
                         ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_rfl_array_v<U>) {
      return ParserRflArray<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_tuple_v<U>) {
      return ParserTuple<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_rfl_tuple_v<U>) {
      return ParserRflTuple<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_pair_v<U>) {
      return ParserPair<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (std::is_enum_v<U>) {
      return ParserEnum<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_wstring_v<U>) {
      return ParserWString<R, W, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_string_view_v<U>) {
      return ParserStringView<R, W, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_commented_v<U>) {
      return ParserCommented<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (std::is_pointer_v<U>) {
      return ParserPtr<R, W, std::remove_cvref_t<std::remove_pointer_t<U>>,
                       ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_c_array_v<U>) {
      using IsCArray = is_c_array<U>;
      return ParserCArray<R, W, typename IsCArray::element_type, IsCArray::size,
                          ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_atomic_v<T>) {
      return ParserAtomic<R, W, U, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (is_atomic_flag_v<T>) {
      return ParserAtomicFlag<R, W, ProcessorsType>::to_schema(_definitions);

    } else if constexpr (rfl::internal::is_description_v<U>) {
      return make_description<U>(_definitions);

    } else if constexpr (rfl::internal::is_deprecated_v<U>) {
      return make_deprecated<U>(_definitions);

    } else if constexpr (std::is_class_v<U> && std::is_aggregate_v<U>) {
      return make_reference<U>(_definitions);

    } else if constexpr (internal::is_literal_v<U>) {
      return Type{Type::Literal{.values_ = U::strings()}};

    } else if constexpr (internal::is_validator_v<U>) {
      return make_validated<U>(_definitions);

    } else if constexpr (internal::has_reflection_type_v<U> ||
                         internal::has_read_reflector<U> ||
                         internal::has_write_reflector<U>) {
      return make_reference<U>(_definitions);

    } else {
      static_assert(rfl::always_false_v<U>, "Unsupported type.");
    }
  }

 private:
  template <class U>
  static schema::Type make_deprecated(
      std::map<std::string, schema::Type>* _definitions) {
    using Type = schema::Type;
    return Type{Type::Deprecated{
        .deprecation_message_ = typename U::DeprecationMessage().str(),
        .description_ = typename U::Content().str(),
        .type_ =
            Ref<Type>::make(Parser<R, W, std::remove_cvref_t<typename U::Type>,
                                   ProcessorsType>::to_schema(_definitions))}};
  }

  template <class U>
  static schema::Type make_description(
      std::map<std::string, schema::Type>* _definitions) {
    using Type = schema::Type;
    return Type{Type::Description{
        .description_ = typename U::Content().str(),
        .type_ =
            Ref<Type>::make(Parser<R, W, std::remove_cvref_t<typename U::Type>,
                                   ProcessorsType>::to_schema(_definitions))}};
  }

  template <class U>
  static schema::Type make_reference(
      std::map<std::string, schema::Type>* _definitions) {
    using Type = schema::Type;
    const auto name = make_type_name<U>();

    if (_definitions->find(name) == _definitions->end()) {
      (*_definitions)[name] =
          Type{Type::Integer{}};  // Placeholder to avoid infinite loop.

      if constexpr (internal::has_reflection_type_v<U>) {
        (*_definitions)[name] =
            Parser<R, W, typename U::ReflectionType, ProcessorsType>::to_schema(
                _definitions);

      } else if constexpr (internal::has_read_reflector<U> ||
                           internal::has_write_reflector<U>) {
        (*_definitions)[name] = Parser<R, W, typename Reflector<U>::ReflType,
                                       ProcessorsType>::to_schema(_definitions);

      } else {
        using NamedTupleType = internal::processed_t<U, ProcessorsType>;
        if constexpr (internal::has_default_val_v<U>) {
          auto t = U{};
          auto view = ProcessorsType::template process<U>(to_view(t));
          (*_definitions)[name] =
              Parser<R, W, NamedTupleType, ProcessorsType>::to_schema(
                  _definitions, &view);
        } else {
          (*_definitions)[name] =
              Parser<R, W, NamedTupleType, ProcessorsType>::to_schema(
                  _definitions);
        }
      }
    }
    return Type{Type::Reference{name}};
  }

  template <class U>
  static schema::Type make_validated(
      std::map<std::string, schema::Type>* _definitions) {
    using Type = schema::Type;
    using ReflectionType = std::remove_cvref_t<typename U::ReflectionType>;
    using ValidationType = std::remove_cvref_t<typename U::ValidationType>;
    return Type{Type::Validated{
        .type_ = Ref<Type>::make(
            Parser<R, W, ReflectionType, ProcessorsType>::to_schema(
                _definitions)),
        .validation_ = ValidationType::template to_schema<ReflectionType>()}};
  }

  /// The way this works is that we allocate space on the stack in this size
  /// of the struct in which we then write the individual fields using views
  /// and placement new. This is how we deal with the fact that some fields
  /// might not be default-constructible.
  static RFL_ALWAYS_INLINE Result<T> read_struct(const R& _r,
                                                 const InputVarType& _var) {
    alignas(T) unsigned char buf[sizeof(T)]{};
    auto ptr = internal::ptr_cast<T*>(&buf);
    auto raw_view = to_view(*ptr);
    auto view = ProcessorsType::template process<T>(std::move(raw_view));
    using ViewType = std::remove_cvref_t<decltype(view)>;
    const auto [set, err] =
        Parser<R, W, ViewType, ProcessorsType>::read_view(_r, _var, &view);
    if (err) [[unlikely]] {
      call_destructors_where_necessary(set, &view);
      return error(err->what());
    }
    auto res = Result<T>(std::move(*ptr));
    call_destructors_where_necessary(set, &view);
    return res;
  }

  /// This is actually more straight-forward than the standard case - we
  /// just allocate a struct and then fill it. But it is less efficient and
  /// it assumes that all values on the struct have a default constructor,
  /// so we only use it when the DefaultIfMissing preprocessor is added.
  static Result<T> read_struct_with_default(const R& _r,
                                            const InputVarType& _var) {
    try {
      auto t = T{};  // This might fail, for instance if the default value does
                     // not satisfy the validator, but in that case we will just
                     // return the error.
      auto view = ProcessorsType::template process<T>(to_view(t));
      using ViewType = decltype(view);
      const auto err =
          Parser<R, W, ViewType, ProcessorsType>::read_view_with_default(
              _r, _var, &view);
      if (err) [[unlikely]] {
        return error(*err);
      }
      return t;
    } catch (std::exception& e) {
      return error(e.what());
    }
  }
};

}  // namespace rfl::parsing

#endif
