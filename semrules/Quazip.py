def accept(path):
    return '/quazip/' in path


def process(path, addPkg):
    addPkg('quazip', None, None, None)
    return 'quazip'

