module;

#ifdef REFLECTCPP_MSGPACK
#include "rfl/msgpack.hpp"
#endif

export module rfl:msgpack;

export namespace rfl::msgpack {
    #ifdef REFLECTCPP_MSGPACK
    using rfl::msgpack::Parser;
    using rfl::msgpack::Reader;
    using rfl::msgpack::Writer;
    using rfl::msgpack::InputObjectType;
    using rfl::msgpack::InputVarType;

    using rfl::msgpack::load;
    using rfl::msgpack::read;
    using rfl::msgpack::save;
    using rfl::msgpack::write;
    #endif
}
