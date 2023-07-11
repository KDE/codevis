# Getting Started

The tool does not visualise C++ source directly: first a code database must
be generated. This can be done from the GUI or from a command line tool.

For the command line tool see the [CLI documentation](doc/command_line_codebase_generation.md).

## Generating a database using the GUI

First, follow the steps in `doc/command_line_codebase_generation.md` to generate
a `compile_commands.json` file for your project. If you are using `bde-tools`,
this file is created in the build configuration step (`cmake_build.py configure`).

Now open the GUI and navigate to "File/New Project From Source Code...". In the dialogue that
pops up, options can be set for the database generation.

The first option is the ignore globs. Any file with a path matching one of these
globs is skipped. For example, using the glob `*.t.cpp` prevents any files with
names ending in `.t.cpp` from being parsed.

It is very important to skip all unit tests (`*.t.cpp`), main functions
(`*.m.cpp`), or other standalone tools (`*standalone*`) because the tool relies
on heavily the one-definition-rule. If you would not link two translation units
into the same binary then they should not be parsed into the same compilation
database.

The recommended setting for the ignore globs is
```
*.t.cpp,*thirdparty*,*standalone*,*.m.cpp
```

The next box allows you to add the path to your `compile_commands.json` file. Use
the Search button to open a file choosing dialogue.

The next box determines the path to the generated database. Once again, the
Search button will open a file choosing dialogue.

The "Overwrite file" toggle controls whether the database file is required to be
new. If it is ticked, any existing database at that path is updated to match
the files as they are currently on disk (see the Incremental Updates section).

The "Parse only physical relations" controls whether the tool will only look at
physical information or if it will also generate logical information. See the
"Physical vs Logical" section for more information.

Once you're ready, press "Apply" to begin database generation. There is a
progress bar to track what is happening. After the physical parse is done the
window can be closed using the "Apply" button again. If a logical parse was also
requested, this will continue in the background even after the window is closed
and the database will be updated with logical information once it becomes ready.

### Physical vs Logical
As discussed in "Large-Scale C++ Volume 1" by John Lakos, the physical design of
software relates to how files are laid out on disk and how they are compiled
and linked into a unit of release. The logical design of software relates to
the layout of the software within the semantics of the programming language.

For example, physical things include
- Components
- Packages
- Package groups
- Physical dependency (`#include` relationships between components) relationships

Logical things include
- Types
- Namespaces
- Uses-In-The- relationships
- Is-A relationships

Creating a database storing only the physical layout of software is relatively
fast because only the layout of files on disk and their `#include`s need to be
processed. A "physical-only" parse is recommended if you are only interested in
this information.

Viewing logical information requires a logical parse (done after a physical
parse). This takes much longer because a full AST has to be generated and type
checked and more information has to be stored about each file.

### Incremental updates
Existing databases can be updated with new information using the "Override file"
toggle.

An already existing database with only physical information can be updated
to include logical information by setting it as the output database, toggling
"Override file" and running a parse. If the existing physical information is
up to date, the physical parse will be skipped: running only the logical parse.

If some files have changed on disk since the database was last generated,
re-running the database generation tool will only re-parse these files and things
which depend upon them. This is similar to how re-compiling after a small change
typically does not lead to rebuilding every translation unit.

## Using the tool
Once you have a database, it can be opened in the tool using
File/Open Code Database. If you just generated the database using the GUI, the
database will have been opened automatically.

On the left side of the tool there is a pane with navigation controls and tables
of information relevant to the current graph. The main body of the tool features
a tabbed interface with the diagram taking up the bulk of the space, with
controls for the diagram arranged down the left side and top.

The navigation pane on the left side is split into "Packages" and "Namespaces"
tabs. The graphs most similar to John's book are those accessed through the
"Packages" tab.

The "Packages" tab gives a tree view starting at each package group in the
database, plus a catch-all "non-lakosian group" for system headers which do not
fall into a lakosian physical hierarchy.

Physical things at any level of the hierarchy can be selected by clicking on them
in the "Packages" tree view: selecting "bal" will load a graph with traversal
starting at "bal", selecting "balb_controlmanager" will draw a graph from this
component. The tree view can be searched using Ctrl+f, / or Edit/Search.

When a graph is loaded it will be zoomed into the main node. The graph can be
zoomed in and out and panned using the controls listed in the bottom left. The
graph can also be scrolled vertically and horizontally using the mouse.

Hover with the mouse more information about any node in the graph. The top line
of the tooltip gives the qualified name of the node, the second line gives the
position of the node in the opposite hierarchy (if the node is a type this will
tell you the package the type belongs to; if it is physical this gives the
prevailing namespace inside that node). This position in the opposite hierarchy
determines a node's colour (making it easy to spot non-cohesive physical and
logical hierarchy).

Some nodes will covered over. Left-clicking on these covered nodes will show
their internal detail. Right-clicking opens a contextual menu with more controls
for a node.

On the left side of the diagram are various tools for interacting with it. At the
top are detail controls for enabling/disabling forward dependencies, reverse
dependencies and the number of edges followed when loading the diagram. See
doc/loading.md for more information about what nodes and edges are or are not
loaded for any particular graph.

Around the midpoint of this toolbar there is a button to show a miniaturised view
of the entire diagram including a visual indication of what portion the current
screen is displaying. This is similar to minimaps found in some RTS games.

On the far left there are tables listing information about the main node of the
graph (for example if you clicked on a package named "bal" then "bal" will be the main node -
the main node is highlighted with a blue edge and its name is used on the tab and
window title). This information includes the complete forward and reverse
dependencies of that node (no matter the graph settings).

## Exporting Images

See Export/Image in the toolbar at the top of the screen. This image will include
the whole graph as currently visible.
