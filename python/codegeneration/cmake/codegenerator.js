var FileHandler = new FileIO();

var alphaNumericRegExp = /^[A-Za-z0-9]+$/;
var CMAKELISTS_ROOT_TEMPLATE = ScriptFolder + "/cmakelists_root.template";
var CMAKELISTS_PKG_GROUP_TEMPLATE = ScriptFolder + "/cmakelists_pkg_group.template";
var CMAKELISTS_PKG_TEMPLATE = ScriptFolder + "/cmakelists_pkg.template";
var HEADER_TEMPLATE = ScriptFolder + "/headerfile.template";
var SOURCE_TEMPLATE = ScriptFolder + "/sourcefile.template";
var rootEntities = new Array();

function _writeTemplateFile(template, build_data, output_file) {
    let contents = FileHandler.render(template, build_data);
    if (FileHandler.hasError()) {
        console.log(FileHandler.errorString())
        return;
    }

    FileHandler.saveFile(output_file, contents);
    if (FileHandler.hasError()) {
        console.log(FileHandler.errorString())
        return;
    }
    console.log("Write Template:", output_file);
}

function buildPkgGroup(pkg_group, output_dir, user_ctx) {
    FileHandler.createPath(output_dir + "/" + pkg_group.name)
    rootEntities.push(pkg_group)

    var build_data = {
        'pkgs': pkg_group.children,
    }

    var pkg_group_dir = output_dir + "/" + pkg_group.name;
    var outputFile = pkg_group_dir + "/CMakeLists.txt";
    _writeTemplateFile(CMAKELISTS_PKG_GROUP_TEMPLATE, build_data, outputFile)
}

function buildPkg(pkg, output_dir, user_ctx) {
    var pkg_group = pkg.parent;
    var pkg_dir = "";

    if (pkg_group) {
        pkg_dir += output_dir + "/" + pkg_group.name + "/" + pkg.name;
    } else {
        pkg_dir += output_dir + "/" + pkg.name;
    }
    FileHandler.createPath(pkg_dir)

    let build_data = {
        'pkg': pkg,
        'deps': _getNonLakosianEntities(pkg.fwdDependencies),
    }

    let outputFile = pkg_dir + "/CMakeLists.txt";
    _writeTemplateFile(CMAKELISTS_PKG_TEMPLATE, build_data, outputFile)
    if (pkg_group == null) {
        rootEntities.push(pkg);
    }
}

function _getNonLakosianEntities(entityArray) {
    let ret = new Array();
    for (const dep of entityArray) {
        if (_isNonLakosianDependency(dep)) {
            ret.push(dep);
        }
    }
    return ret;
}

function _isNonLakosianDependency(entity) {
    if (entity.type == "PackageGroup") {
        return entity.name == 'non-lakosian group';
    }
    if (entity.parent) {
        return _isNonLakosianDependency(entity.parent);
    }
    return false;
}

/* JS Lacks isAlphaNumeric, so let's add it. */
function isAlphaNumeric(Value) {
    return (Value.match(alphaNumericRegExp));
}

function buildComponent(component, output_dir, user_ctx) {
    var pkg = component.parent
    var pkg_group = pkg.parent
    var pkg_dir = output_dir;
    if (pkg_group) {
        pkg_dir += "/" + pkg_group.name + "/" + pkg.name;
    } else {
        pkg_dir += "/" + pkg.name;
    }

    var component_basename = pkg_dir + "/" + component.name;

    var build_data = {
        'component_name': component.name,
        'package_name': pkg.name,
        'component_fwd_dependencies': _getNonLakosianEntities(component.fwdDependencies),
        'should_generate_namespace': isAlphaNumeric(pkg.name),
    };

    _writeTemplateFile(HEADER_TEMPLATE, build_data, component_basename + '.h');
    _writeTemplateFile(SOURCE_TEMPLATE, build_data, component_basename + '.cpp');
}

export function beforeProcessEntities(output_dir, user_ctx) {
    rootEntities = new Array();
}

export function buildPhysicalEntity(entity, output_dir, user_ctx) {
    switch(entity.type){
        case "PackageGroup":
            buildPkgGroup(entity, output_dir);
        break;
        case "Package":
            buildPkg(entity, output_dir);
        break;
        case "Component":
            buildComponent(entity, output_dir);
        break;
    }
}

export function afterProcessEntities(output_dir, user_ctx) {
    var build_data = {
        'root_entities': rootEntities
    };

    var outputFile = output_dir + "/CMakeLists.txt";
    _writeTemplateFile(CMAKELISTS_ROOT_TEMPLATE, build_data, outputFile)
}

