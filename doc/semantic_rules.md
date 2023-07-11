# Semantic Rules

"Semantic Rules" are a way to provide more information about the system while the code analysis tool is being run. This
enables you to avoid having things placed in the `non-lakosian` automatic group. This is an important feature for
codebases that doesn't follow the lakosian rules described in John Lakos book or if you have a different project
hierarchy between the installed libraries and the source code.

To use the semantic rules feature, the software must be compiled with Python support (Which it is the default). To manually
enable Python support, pass the flag `-DENABLE_PYTHON_PLUGINS=ON` to CMake.

The tool will search for `.py` files in the following places:

- The path defined in the env var `$SEMRULES_PATH`
- In a folder named `semrules` in the application path
- In a folder named `semrules` in user's home directory

For each file found in the code analysis step, the software will search and execute the function `accept(path)` in each
python file in all the paths mentioned. If this function returns `True`, then the software will search and execute a
function called `process(path, addPkg)`.

You can find examples of Python scripts on the `semrules/` folder in the root of the project.
