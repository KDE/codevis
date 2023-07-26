from conans import ConanFile, CMake

# Todo: Add dbus as a dependency, but conan is
# currently failing on that.
class DiagramServerConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    generators = (
        "cmake_find_package",
        "cmake_paths",
    )

    default_options = {
        "qt:qttools":True,
        "qt:shared":True,
        "qt:widgets":True,
        "qt:gui":True,
        "qt:qtsvg":True,
        # TODO: Install Python2 on CI so Conan is able to compile Qt with those
        # "qt:qtwebengine":True,
        # "qt:qtdeclarative":True,
        # "qt:qtlocation":True,
        # "qt:qtwebchannel":True,
    }

    requires = (
        "freetype/2.11.1",
        "sqlite3/3.37.2",
        "qt/5.15.2",
        "openssl/1.1.1q",
        "catch2/2.13.8",
        "extra-cmake-modules/5.84.0",
        "zlib/1.2.12",
        "libiconv/1.17",
        "libpng/1.6.38",
    )

    # There's an error currently with libxml2 on windows.
    def requirements(self):
        if self.settings.os != "Windows":
            self.requires("libxml2/2.9.12")
