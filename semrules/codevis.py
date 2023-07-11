# Only some specific files need to be cleaned for our project. That's why we do not run 'accept' in all the files.
ACCEPT_NAMES = [
    'version',
    'preferences',
    'qrc_resources',
]


def is_valid_build_path(path):
    import os
    dirname = os.path.dirname(path)
    return os.path.isfile(f'{dirname}/codevispreferences_export.h')


def is_valid_src_path(path):
    import os
    dirname = os.path.dirname(path)
    return os.path.isfile(f'{dirname}/codevis.desktop')


def accept(path):
    if 'diagram-server/desktopapp/' in path:
        return True

    if '/codevisadaptor' in path:
        return True

    if not is_valid_build_path(path) and not is_valid_src_path(path):
        return False
    for f in ACCEPT_NAMES:
        if f in path:
            return True
    return False


def process(path, addPkg):
    if 'desktopapp' in path:
        pkgName = 'lvt-desktopapp'
    elif '/codevisadaptors' in path:
        pkgName = 'lvt-desktopapp'
    else:
        pkgName = 'lvt-buildfiles'

    addPkg(pkgName, None, None, None)
    return pkgName
