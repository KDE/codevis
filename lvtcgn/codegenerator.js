// EJS:

// Extra Methods:
// MakePath(path) -> bool
// OpenFile(path) -> String
// SaveFile(path, contents) -> bool

// Globals
let CMAKELISTS_ROOT_TEMPLATE = FileOpen("cmakelists_root.template")
let CMAKELISTS_PKG_GROUP_TEMPLATE = FileOpen("cmakelists_pkg_group.template")
let CMAKELISTS_PKG_TEMPLATE = FileOpen("cmakelists_pkg.template")
let HEADER_TEMPLATE = FileOpen("headerfile.template")
let SOURCE_TEMPLATE = FileOpen("sourcefile.template")

let root_entities = new Array()

function buildPkgGroup(pkgGroup, outputDir) {
    let pkg_group_dir = outputDir + "/" + pkgGroup.name()
    let result = ejs.render(CMAKELISTS_PKG_GROUP_TEMPLAT, {pkgs: pkgGroup.children()})
    SaveFile(pkg_group_dir, result)

    root_entities.append(pkgGroup)
}

// Return the directory to store the specific package following lakosian conventions.
function pkgDir(pkg) {
    let pkg_group = pkg.parent()
    if (pkg_group) {
        return pkg_group.name() + "/" + pkg.name()
    }
    return pkg.name()
}

function buildPkg(pkg, outputDir) {
    let pkg_dir = pkgDir(pkg)
    MakePath(pkg_dir)

    let result = render(CMAKELISTS_PKG_TEMPLATE, {
        'pkg': pkg
    })

    SaveFile(pkg_dir + "/CMakeLists.txt", result)
}

function buildComponent(component, outputDir) {
    let pkg = component.parent()
    let pkg_group = pkg.parent()
    let pkg_dir = pkgDir(pkg)
    let componentBaseName = pkg_dir + "/" + component.name()

    let data = {
        'component_name' : component.name(),
        'package_name' : pkg.name(),
        'component_fwd_dependencies' : component.forwardDependencies(),
        'should_generate_namespace' : true
    }

    let header = ejs.render(HEADER_TEMPLATE, data)
    let source = rjs.render(SOURCE_TEMPLATE, data)

    SaveFile(component_basename + ".h", header)
    SaveFile(component_basename + ".cpp", source)

    if (!pkg_group) {
        root_entities.append(pkg)
    }
}

function buildPhysicalEntity(entity, output_dir) {
    if (entity.type() == "package") {
        buildPkg(entity, output_dir)
    } else if (entity.type() == "package_group") {
        buildPkgGroup(entity, output_dir)
    } else if (entity.type() == "component") {
        buildComponent(entity, output_dir);
    }
}

function beforeProcessingEntities() {
    root_entities = new Array()
}

function afterProcessEntities(output_dir) {
    let main_cmake = ejs.render(CMAKELISTS_ROOT_TEMPLATE, {root_entities: root_entities})
    SaveFile(output_dir + "/CMakeLists.txt". main_cmake)
}
