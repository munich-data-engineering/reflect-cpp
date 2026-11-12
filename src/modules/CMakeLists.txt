cmake_minimum_required(VERSION 3.28)

add_library(reflectcpp_module)

set(MODULE_FILES)

if(REFLECTCPP_JSON)
    list(APPEND MODULE_FILES rfl.json.cppm)
    target_compile_definitions(reflectcpp_module PUBLIC REFLECTCPP_JSON)
endif()

if(REFLECTCPP_AVRO)
    list(APPEND MODULE_FILES rfl.avro.cppm)
    target_compile_definitions(reflectcpp_module PUBLIC REFLECTCPP_AVRO)
endif()

if(REFLECTCPP_BSON)
    list(APPEND MODULE_FILES rfl.bson.cppm)
    target_compile_definitions(reflectcpp_module PUBLIC REFLECTCPP_BSON)
endif()

if(REFLECTCPP_CAPNPROTO)
    list(APPEND MODULE_FILES rfl.capnproto.cppm)
    target_compile_definitions(reflectcpp_module PUBLIC REFLECTCPP_CAPNPROTO)
endif()

if(REFLECTCPP_CBOR)
    list(APPEND MODULE_FILES rfl.cbor.cppm)
    target_compile_definitions(reflectcpp_module PUBLIC REFLECTCPP_CBOR)
endif()

if(REFLECTCPP_CSV)
    list(APPEND MODULE_FILES rfl.csv.cppm)
    target_compile_definitions(reflectcpp_module PUBLIC REFLECTCPP_CSV)
endif()

if(REFLECTCPP_FLEXBUFFERS)
    list(APPEND MODULE_FILES rfl.flexbuf.cppm)
    target_compile_definitions(reflectcpp_module PUBLIC REFLECTCPP_FLEXBUFFERS)
endif()

if(REFLECTCPP_MSGPACK)
    list(APPEND MODULE_FILES rfl.msgpack.cppm)
    target_compile_definitions(reflectcpp_module PUBLIC REFLECTCPP_MSGPACK)
endif()

if(REFLECTCPP_PARQUET)
    list(APPEND MODULE_FILES rfl.parquet.cppm)
    target_compile_definitions(reflectcpp_module PUBLIC REFLECTCPP_PARQUET)
endif()

if(REFLECTCPP_TOML)
    list(APPEND MODULE_FILES rfl.toml.cppm)
    target_compile_definitions(reflectcpp_module PUBLIC REFLECTCPP_TOML)
endif()

if(REFLECTCPP_UBJSON)
    list(APPEND MODULE_FILES rfl.ubjson.cppm)
    target_compile_definitions(reflectcpp_module PUBLIC REFLECTCPP_UBJSON)
endif()

if(REFLECTCPP_XML)
    list(APPEND MODULE_FILES rfl.xml.cppm)
    target_compile_definitions(reflectcpp_module PUBLIC REFLECTCPP_XML)
endif()

if(REFLECTCPP_YAML)
    list(APPEND MODULE_FILES rfl.yaml.cppm)
    target_compile_definitions(reflectcpp_module PUBLIC REFLECTCPP_YAML)
endif()

list(APPEND MODULE_FILES rfl.cppm)

target_sources(reflectcpp_module
    PUBLIC
        FILE_SET CXX_MODULES FILES ${MODULE_FILES}
)

target_compile_features(reflectcpp_module PUBLIC cxx_std_23)

target_include_directories(reflectcpp_module PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(reflectcpp_module PUBLIC reflectcpp::reflectcpp)

add_library(reflectcpp::module ALIAS reflectcpp_module)

# Installation
install(TARGETS reflectcpp_module
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
   RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/src/module
)

if (REFLECTCPP_BUILD_TESTS)
	add_executable(reflect-cpp-modules-tests)
	target_include_directories(reflect-cpp-modules-tests PUBLIC
	    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
	    $<INSTALL_INTERFACE:include>
	)
	target_sources(reflect-cpp-modules-tests PUBLIC "tests.cpp")
	target_sources(reflect-cpp-modules-tests
	    PUBLIC
	    FILE_SET all_my_modules TYPE CXX_MODULES FILES ${MODULE_FILES} 
	)
endif()
