import re

def accept(path):
    return '/micro16/' in path


def process(path, addPkg):
    path = path.lower()
    pkgName = 'Micro16'
    addPkg(pkgName, None, None, None)
    pkgName = processSourceDir(path, pkgName, addPkg)
    return pkgName


def processSourceDir(path, pkgName, addPkg):
    for innerPkg in re.split('src', path)[-1].split('/')[1:-1]:
        addPkg(innerPkg, pkgName, None, None)
        pkgName = innerPkg
    return pkgName

