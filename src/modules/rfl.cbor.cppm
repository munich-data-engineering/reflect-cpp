module;

#ifdef REFLECTCPP_CBOR
#include "rfl/cbor.hpp"
#endif

export module rfl:cbor;

export namespace rfl::cbor {
    #ifdef REFLECTCPP_CBOR
    using rfl::cbor::Parser;
    using rfl::cbor::Reader;
    using rfl::cbor::Writer;
    using rfl::cbor::InputObjectType;
    using rfl::cbor::InputVarType;

    using rfl::cbor::load;
    using rfl::cbor::read;
    using rfl::cbor::save;
    using rfl::cbor::write;
    #endif
}
