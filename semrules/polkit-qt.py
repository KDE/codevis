def accept(path):
    import re
    return re.search('/polkit-qt([0-9]+)?-?([0-9]+)?/', path) is not None


def process(path, addPkg):
    addPkg('PolkitQt', None, None, None)
    return 'PolkitQt'

