from conan import ConanFile
from conan.tools.files import get, copy, rmdir
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


from conan.tools.env import VirtualBuildEnv
from conan.tools.build import check_min_cppstd
from conan.tools.scm import Version
from conan.errors import ConanInvalidConfiguration

import os

required_conan_version = ">=1.54"


class ReflectCppConan(ConanFile):
    name = "reflectcpp"
    description = "C++-20 library for fast serialization, deserialization and validation using reflection"
    license = "MIT"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://github.com/munich-data-engineering/reflect-cpp"
    topics = (
        "reflection",
        "serialization",
        "memory",
        "Cap'n Proto",
        "cbor",
        "flatbuffers",
        "json",
        "msgpack",
        "toml",
        "xml",
        "yaml",
    )
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"

    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_boost_serialization": [True, False],
        "with_capnproto": [True, False],
        "with_cbor": [True, False],
        "with_cereal": [True, False],
        "with_csv": [True, False],
        "with_flatbuffers": [True, False],
        "with_msgpack": [True, False],
        "with_parquet": [True, False],
        "with_toml": [True, False],
        "with_ubjson": [True, False],
        "with_xml": [True, False],
        "with_yaml": [True, False],
        "with_yas": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_boost_serialization": False,
        "with_capnproto": False,
        "with_cbor": False,
        "with_cereal": False,
        "with_csv": False,
        "with_flatbuffers": False,
        "with_msgpack": False,
        "with_parquet": False,
        "with_toml": False,
        "with_ubjson": False,
        "with_xml": False,
        "with_yaml": False,
        "with_yas": False,
    }

    def config_options(self):
        if self.settings.os == "Windows":
            self.options.rm_safe("fPIC")

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def requirements(self):
        self.requires("ctre/3.9.0", transitive_headers=True)
        self.requires("enchantum/0.5.0", transitive_headers=True)
        self.requires("yyjson/0.10.0", transitive_headers=True)
        if self.options.with_boost_serialization:
            self.requires("boost/1.90.0", transitive_headers=True)
        if self.options.with_capnproto:
            self.requires("capnproto/1.1.0", transitive_headers=True)
        if self.options.with_cbor or self.options.with_ubjson:
            self.requires("jsoncons/0.176.0", transitive_headers=True)
        if self.options.with_cereal:
            self.requires("cereal/1.3.2", transitive_headers=True)
        if self.options.with_csv or self.options.with_parquet:
            self.requires("arrow/21.0.0", transitive_headers=True)
        if self.options.with_flatbuffers:
            self.requires("flatbuffers/24.3.25", transitive_headers=True)
        if self.options.with_msgpack:
            self.requires("msgpack-c/6.0.0", transitive_headers=True)
        if self.options.with_toml:
            self.requires("tomlplusplus/3.4.0", transitive_headers=True)
        if self.options.with_xml:
            self.requires("pugixml/1.15", transitive_headers=True)
        if self.options.with_yaml:
            self.requires("yaml-cpp/0.8.0", transitive_headers=True)
        if self.options.with_yas:
            self.requires("yas/7.1.0", transitive_headers=True)

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.23 <4]")

    def validate(self):
        if self.settings.get_safe("compiler.cppstd"):
            check_min_cppstd(self, self._min_cppstd)

    def layout(self):
        cmake_layout(self, src_folder=".")

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        env = VirtualBuildEnv(self)
        env.generate()
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.cache_variables["REFLECTCPP_BUILD_SHARED"] = self.options.shared
        tc.cache_variables["REFLECTCPP_USE_BUNDLED_DEPENDENCIES"] = False
        tc.cache_variables["REFLECTCPP_USE_VCPKG"] = False
        tc.cache_variables["REFLECTCPP_BOOST_SERIALIZATION"] = (
            self.options.with_boost_serialization
        )
        tc.cache_variables["REFLECTCPP_CAPNPROTO"] = self.options.with_capnproto
        tc.cache_variables["REFLECTCPP_CBOR"] = self.options.with_cbor
        tc.cache_variables["REFLECTCPP_CEREAL"] = self.options.with_cereal
        tc.cache_variables["REFLECTCPP_CSV"] = self.options.with_csv
        tc.cache_variables["REFLECTCPP_FLEXBUFFERS"] = self.options.with_flatbuffers
        tc.cache_variables["REFLECTCPP_MSGPACK"] = self.options.with_msgpack
        tc.cache_variables["REFLECTCPP_PARQUET"] = self.options.with_parquet
        tc.cache_variables["REFLECTCPP_TOML"] = self.options.with_toml
        tc.cache_variables["REFLECTCPP_XML"] = self.options.with_xml
        tc.cache_variables["REFLECTCPP_YAML"] = self.options.with_yaml
        tc.cache_variables["REFLECTCPP_YAS"] = self.options.with_yas
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(
            self,
            pattern="LICENSE",
            dst=os.path.join(self.package_folder, "licenses"),
            src=self.source_folder,
        )
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

    def package_info(self):
        self.cpp_info.libs = ["reflectcpp"]

    @property
    def _min_cppstd(self):
        return 20

    @property
    def _compilers_minimum_version(self):
        return {
            "Visual Studio": "17",
            "msvc": "1938",
            "gcc": "11",
            "clang": "13",
            "apple-clang": "15",
        }
