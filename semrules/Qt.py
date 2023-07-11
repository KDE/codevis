import re

def accept(path):
    return '/qt' in path


def process(path, addPkg):
    path = path.lower()

    pkgName = "Qt"
    addPkg(pkgName, None, None, None)

    for innerPkg in re.split('/qt5', path)[-1].split('/')[1:-1]:
        addPkg(innerPkg, pkgName, None, None)
        pkgName = innerPkg

    return pkgName
