def accept(path):
    return '/boost/' in path


def process(path, addPkg):
    addPkg('Boost', None, None, None)
    return 'Boost'

