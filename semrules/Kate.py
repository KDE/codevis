REPOSITORY_NAME = 'Kate'

def accept(path):
    return '/kate/' in path


def process(path, addPkg):
    import re

    if 'addons' in path:
        pkgGrpName = 'addons'
        match = re.match(".*kate/addons/(.*?)/.*", path)
        pkgName = match.group(1) if match else 'Unknown'
    elif 'apps' in path:
        pkgGrpName = 'apps'
        match = re.match(".*kate/apps/(.*?)/.*", path)
        pkgName = match.group(1) if match else 'Unknown'
    else:
        pkgGrpName = None
        pkgName = 'etc'

    addPkg('Kate', None, REPOSITORY_NAME, None)
    addPkg(pkgGrpName, 'Kate', None, None)
    addPkg(pkgName, pkgGrpName, None, None)
    return pkgName

