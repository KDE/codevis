REPOSITORY_NAME = 'Konsole'

def accept(path):
    return '/konsole/' in path


def process(path, addPkg):
    import re

    pkgName = "konsole"
    addPkg(pkgName, None, REPOSITORY_NAME, None)
    for innerPkg in re.split('src', path)[-1].split('/')[1:-1]:
        addPkg(innerPkg, pkgName, None, None)
        pkgName = innerPkg

    return pkgName

