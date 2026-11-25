module;

#ifdef REFLECTCPP_CAPNPROTO
#include "rfl/capnproto.hpp"
#include "rfl/capnproto/is_named_type.hpp"
#include "rfl/capnproto/schema/CapnProtoTypes.hpp"
#endif

export module rfl:capnproto;

export namespace rfl::capnproto {
    #ifdef REFLECTCPP_CAPNPROTO
    using rfl::capnproto::Parser;
    using rfl::capnproto::Reader;
    using rfl::capnproto::Schema;
    using rfl::capnproto::SchemaImpl;
    using rfl::capnproto::SchemaHolder;
    using rfl::capnproto::Writer;
    using rfl::capnproto::InputObjectType;
    using rfl::capnproto::InputVarType;

    using rfl::capnproto::load;
    using rfl::capnproto::read;
    using rfl::capnproto::save;
    using rfl::capnproto::to_schema;
    using rfl::capnproto::write;
    using rfl::capnproto::get_root_name;
    using rfl::capnproto::is_named_type;
    using rfl::capnproto::to_string_representation;
    using rfl::capnproto::to_schema;

    namespace schema {
        using rfl::capnproto::schema::CapnProtoTypes;
        using rfl::capnproto::schema::Type;

        using rfl::capnproto::schema::MAP_DEFINITION;

        using rfl::capnproto::schema::operator<<;
    }
    #endif
}
