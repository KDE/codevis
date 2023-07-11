def accept(path):
    return f'/sqlite3.h' in path


def process(path, addPkg):
    addPkg('sqlite3', None, None, None)
    return 'sqlite3'

