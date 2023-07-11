def accept(path):
    return '/catch2/' in path


def process(path, addPkg):
    addPkg('Catch2', None, None, None)
    return 'Catch2'

