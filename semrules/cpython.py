def accept(path):
    import re
    return re.search('/python([0-9]+)?.?([0-9]+)?/', path) is not None


def process(path, addPkg):
    addPkg('CPython', None, None, None)
    return 'CPython'

