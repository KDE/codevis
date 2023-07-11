import sys
from pathlib import Path
sys.path.append(str(Path(__file__).parent.resolve()) + "/../../site-packages/")

from pathlib import Path
import jinja2

# Template files are read globally so it's contents can be shared between calls
with open(Path(__file__).parent.resolve() / 'cmakelists_root.template', 'r') as f:
    CMAKELISTS_ROOT_TEMPLATE = f.read()
with open(Path(__file__).parent.resolve() / 'cmakelists_pkg_group.template', 'r') as f:
    CMAKELISTS_PKG_GROUP_TEMPLATE = f.read()
with open(Path(__file__).parent.resolve() / 'cmakelists_pkg.template', 'r') as f:
    CMAKELISTS_PKG_TEMPLATE = f.read()
with open(Path(__file__).parent.resolve() / 'headerfile.template', 'r') as f:
    HEADER_TEMPLATE = f.read()
with open(Path(__file__).parent.resolve() / 'sourcefile.template', 'r') as f:
    SOURCE_TEMPLATE = f.read()


def _writeTemplateFile(template, build_data, output_file):
    '''
    Helper function for writing contents from a given template file
    '''
    with open(Path(output_file), 'w') as outfile:
        outfile.write(jinja2.Template(template).render(build_data))


def buildPkgGroup(pkg_group, output_dir, user_ctx):
    '''
    Ensures the given package group exists in the filesystem
    '''
    pkg_group_dir = f'{output_dir}/{pkg_group.name()}'
    Path(pkg_group_dir).mkdir(parents=True, exist_ok=True)

    user_ctx['root_entities'].append(pkg_group)

    build_data = {
        'pkgs': pkg_group.children(),
    }

    _writeTemplateFile(CMAKELISTS_PKG_GROUP_TEMPLATE, build_data, f'{pkg_group_dir}/CMakeLists.txt')


def buildPkg(pkg, output_dir, user_ctx):
    '''
    Ensures the given package exists in the filesystem and create its basic structure
    '''
    pkg_group = pkg.parent()
    if pkg_group:
        pkg_dir = f'{output_dir}/{pkg_group.name()}/{pkg.name()}'
    else:
        pkg_dir = f'{output_dir}/{pkg.name()}'
    Path(pkg_dir).mkdir(parents=True, exist_ok=True)

    build_data = {
        'pkg': pkg,
        'deps': [
            dep for dep in pkg.forwardDependencies()
            if not _isNonLakosianDependency(dep)
        ],
    }

    _writeTemplateFile(CMAKELISTS_PKG_TEMPLATE, build_data, f'{pkg_dir}/CMakeLists.txt')
    if pkg_group is None:
        user_ctx['root_entities'].append(pkg)


def _isNonLakosianDependency(entity):
    if entity.type() == pycgn.DiagramType.PackageGroup:
        return entity.name() == 'non-lakosian group'
    if entity.parent():
        return _isNonLakosianDependency(entity.parent())
    return False


def buildComponent(component, output_dir, user_ctx):
    '''
    Build the C++ component in the filesystem with header, source and test files
    '''
    pkg = component.parent()
    assert pkg, f"Package for entity {component.name()} not found."
    pkg_group = pkg.parent()
    if pkg_group:
        pkg_dir = f'{output_dir}/{pkg_group.name()}/{pkg.name()}'
    else:
        pkg_dir = f'{output_dir}/{pkg.name()}'
    component_basename = f'{pkg_dir}/{component.name()}'

    build_data = {
        'component_name': component.name(),
        'package_name': pkg.name(),
        'component_fwd_dependencies': [
            dep.name() for dep in component.forwardDependencies()
            if not _isNonLakosianDependency(dep)
        ],
        'should_generate_namespace': pkg.name().isalnum(),
    }

    _writeTemplateFile(HEADER_TEMPLATE, build_data, component_basename + '.h')
    _writeTemplateFile(SOURCE_TEMPLATE, build_data, component_basename + '.cpp')


def beforeProcessEntities(output_dir, user_ctx):
    user_ctx['root_entities'] = []


def buildPhysicalEntity(pycgn_module, entity, output_dir, user_ctx):
    '''
    API called from DiagramViewer software.
    See software docs for details.
    '''
    global pycgn
    pycgn = pycgn_module

    BUILD_DISPATCH = {
        pycgn.DiagramType.PackageGroup: buildPkgGroup,
        pycgn.DiagramType.Package: buildPkg,
        pycgn.DiagramType.Component: buildComponent
    }
    run = BUILD_DISPATCH.get(entity.type(), lambda _1, _2, _3: None)
    run(entity, output_dir, user_ctx)


def afterProcessEntities(output_dir, user_ctx):
    build_data = {
        'root_entities': user_ctx['root_entities'],
    }

    _writeTemplateFile(CMAKELISTS_ROOT_TEMPLATE, build_data, f'{output_dir}/CMakeLists.txt')
