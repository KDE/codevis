# compile_commands.json

A build folder with a `compile_commands.json` file is necessary to load a codebase, 
this will tell us all the compiler commands used to generate all binaries and how they
interconnect, we will use this information to extract the data from your sources.

## CMake:

Add option `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` to `cmake` while configuring the project.

Note that not all generators accept this flag. If you are using `MSVC` with CMake, use the `Ninja` generator. 
It can be either manually installed and added to `%PATH%` or downloaded using 
[clang-power-tools](https://github.com/Caphyon/clang-power-tools) plugin on visual studio code.

If you are on `linux`, `mac`, `msys` or `mingw`, use either the default, or the Ninja generator.

## Meson:

Meson automatically generates the `compile_commands.json` on the build directory

## GNU Makefiles / Autotools:

If your project is based on autotools or Gnu Makefiles, it is possible to use 
[Bear](https://github.com/rizsotto/Bear) tool to extract `compile_commands.json`.

# Non-lakosian folders

The code extractor process tries it's best to separate the sources it finds in lakosian package groups. 
But not everything on a project will be lakosian (for instance, third party libraries), so you need to
specify those manually unless you want them to be (roughly) translated to Lakosian Packages.

To avoid packages being placed in the `non-lakosian` automatic group, you can set up [Semantic Rules](semantic_rules.md) to change
the behavior of how things are parsed.

# Start the parse

The application will halt for a few minutes, while parsing happens in the background. The duration of the parsing phase is _less_ 
than a software compilation, but we will be running multiple instances of a compiler in parallel to extract every possible data
that we can. So if you have an enormous software, this will also take the time needed.