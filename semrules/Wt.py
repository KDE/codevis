def accept(path):
    return '/Wt/' in path


def process(path, addPkg):
    addPkg('Wt', None, None, None)
    return 'Wt'

