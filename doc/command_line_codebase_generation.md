
# Run Instructions

There are two ways to run the codebase parse: The GUI and the command line
interface. In this document we will discuss the command line interface.

The `codevis_create_codebase_db` tool takes a `compile_commands.json` file as input and
outputs an SQLite code database. Multiple `compile_commands.json` can be
specified to generate a database spanning multiple projects.

Run `codevis_create_codebase_db --help` to see all available parameters. Some parameters are summarized here:

- `-j` option can be used to use multiple threads to parse the codebase. This
is similar to the `-j` option to `make` and is strongly advised.

- `--update` allows the updating of an existing database. This is done
incrementally so only files which have been updated since that database was
generated are re-parsed.

- `--physical-only` can be used to skip parsing for logical relationships. The
physical parse is considerably faster than the logical parse so this is a good
idea for situations where no logical relationships or classes are needed in the
diagram.

- `--ignore` option takes a glob describing file paths to skip. For example
using `--ignore *.t.cpp` will skip all unit tests. It is strongly recommended to
specify at least `--ignore *.t.cpp --ignore *.m.cpp --ignore *standalone*` so
that multiple definitions of the main() (or other things) in each file cannot
cause inconsistencies in the database. In general, the database relies on the
one-definition-rule (as does the C++ linker). If two translation units are not
intended to be linked into the same binary, they should not be included
in the same code database.

- `--compile-command` Ingests a *single* entry of the compile commands file in 
the form of a JSON object with `file`, `directory`, `command`, `output` keys, and 
produces a database file with *just* the contents of this translation unit (plus used headers).
if you used the `--compile-commands` flag, you will need to merge the 
resulting databases into a single one later on (just like you need to run a linter on multiple
object files to produce a binary). for that we have the tool `codevis_merge_databases`

# CLI run example

For this example, Bloomberg's Open Source BDE code will be used as a target. BDE uses a meta-build system (bde-tools) to configure arguments and the
environment for cmake. Information about configuring and building BDE can be found at

https://bloomberg.github.io/bde/library_information/build.html

A full BDE build is not required to generate the database. We only need to
configure and run cmake.

1. Checkout the bde-tools repo

        $ git clone https://github.com/bloomberg/bde-tools.git

2. Add the bde-tools/bin directory to your $PATH to allow easier access to the helper scripts.

3. Checkout the bde repo

        $ git clone https://github.com/bloomberg/bde.git

This command sets environment variables that define effective ufid/uplid, compiler and build
directory for subsequent commands. Where possible, it is a good idea to configure the BDE
build to use  clang (as we parse the codebase using clang). Configurations for other
compilers have been tested and work currently, but may not in the future if some non-clang
compiler extensions are used.

3. 1 (Optional) To see which compilers are available on your platform:

        $ bde_build_env.py list

Output should look something like

```
Available compilers:
 0: gcc-11.2.0 (default)
     CXX: /usr/lib/ccache/g++
     CC : /usr/lib/ccache/gcc
     Toolchain: gcc-default
 1: gcc-11.2.0 
     CXX: /usr/lib/ccache/g++-11
     CC : /usr/lib/ccache/gcc-11
     Toolchain: gcc-default
 2: clang-13.0.0 
     CXX: /usr/lib/ccache/clang++
     CC : /usr/lib/ccache/clang
     Toolchain: clang-default
```

        $ eval `bde_build_env.py --build-type=Release -c clang-13.0.0`

4. Configure bde (a fully build is not needed)

        $ cmake_build.py configure

You should now have the `compile_commands.json` file in the bde build directory
for your target/compiler configuration.

5. Run the tool against the BDE source to generate a database:

        $ create_codebase_db --compile-commands-json /path/to/bde/build --source-path /path/to/bde -j4

The tool will then output a raw database file, which can be converted to a project file using the `codevis_create_prj_from_db` CLI tool.


## Troubleshooting

Whilst the parser is running you may see messages about missing header files;
```
/usr/include/wchar.h:35:10: fatal error: 'stddef.h' file not found
#include <stddef.h>
         ^~~~~~~~~~
```

This occurs when clang can't find its own standard library headers. Try the GUI
via one of the application bundles (e.g. appimage), as these should distribute
the headers with the application.
