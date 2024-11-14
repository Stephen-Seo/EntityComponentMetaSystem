from conan import ConanFile
from conan.tools.files import copy


class ECMSConan(ConanFile):
    name = "ecms"
    version = "1.0"
    # No settings/options are necessary, this is header only
    exports_sources = "src/EC/*"
    no_copy_source = True

    def package(self):
        # This will also copy the "include" folder
        copy(self, "*.hpp", self.source_folder, self.package_folder)

    def package_info(self):
        # For header-only packages, libdirs and bindirs are not used
        # so it's recommended to set those as empty.
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.includedirs = ['src']
