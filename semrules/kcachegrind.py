REPOSITORY_NAME = 'kcachegrind'

def accept(path):
    return f'/{REPOSITORY_NAME}/' in path


def process(path, addPkg):
    import re

    pkgName = REPOSITORY_NAME
    addPkg(pkgName, None, REPOSITORY_NAME, None)
    for innerPkg in re.split('src', path)[-1].split('/')[1:-1]:
        addPkg(innerPkg, pkgName, None, None)
        pkgName = innerPkg

    return pkgName

