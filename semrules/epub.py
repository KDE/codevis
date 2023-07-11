PKG_AS_PATH = 'epub'

def accept(path):
    return f'/epub.h' in path or f'/epub_shared.h' in path


def process(path, addPkg):
    addPkg(f'{PKG_AS_PATH}', None, None, None)
    return f'{PKG_AS_PATH}'

