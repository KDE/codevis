PKG_AS_PATH = 'configuration-parser'

def accept(path):
    return f'/{PKG_AS_PATH}/' in path


def process(path, addPkg):
    addPkg(f'{PKG_AS_PATH}', None, None, None)
    return f'{PKG_AS_PATH}'

