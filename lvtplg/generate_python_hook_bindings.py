import sys
from hooks import HOOKS


if __name__ == "__main__":
    # Note: Please don't use JINJA for code generation, since some users don't have it installed.
    output_path = sys.argv[1]
    filename = 'hookbindings.inc.cpp'

    contents = []
    contents.append("// This file is automatically generated. Do not modify it directly - Use the generator file instead.")
    contents.append('')
    contents.append('#include <iostream>')

    for hook in HOOKS:
        contents.append(f'static void {hook.name}Wrapper({hook.handler} *handler)')
        contents.append(f'{{')
        contents.append(f'    auto& module = *PythonLibraryDispatcher::PyResolveContext::activeModule;')
        contents.append(f'    auto func = module.attr("hook{hook.name}");')
        contents.append(f'    auto pyLksPlugin = py::module_::import("pyLksPlugin");')
        contents.append(f'    (void) pyLksPlugin;')
        contents.append(f'    try {{')
        contents.append(f'        func(handler);')
        contents.append(f'    }} catch (py::error_already_set const& e) {{')
        contents.append(f'        std::cout << e.what() << "\\n";')
        contents.append(f'        py::module::import("traceback").attr("print_exception")(e.type(), e.value(), e.trace());')
        contents.append(f'    }}')
        contents.append(f'}}')
        contents.append(f'')

    contents.append('py::module_ *PythonLibraryDispatcher::PyResolveContext::activeModule = nullptr;')
    contents.append('PythonLibraryDispatcher::PyResolveContext::PyResolveContext(py::module_& module, const std::string& hookName)')
    contents.append(' : AbstractLibraryDispatcher::ResolveContext(nullptr)')
    contents.append('{')
    contents.append('    PythonLibraryDispatcher::PyResolveContext::activeModule = &module;')
    contents.append('    if (py::hasattr(module, hookName.c_str())) {')
    for hook in HOOKS:
        contents.append(f'        if (hookName == "hook{hook.name}") {{')
        contents.append(f'            this->hook = (functionPointer) (&{hook.name}Wrapper);')
        contents.append(f'        }}')
    contents.append('    }')
    contents.append('}')

    with open(output_path + "/ct_lvtplg_" + filename, 'w') as f:
        f.write('\n'.join(contents))
