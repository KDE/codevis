def accept(path):
    import re
    return re.search('/llvm(-?[0-9]+)?/', path) is not None or re.search('/llvm-c(-?[0-9]+)?/', path) is not None


def process(path, addPkg):
    addPkg('LLVM', None, None, None)
    return 'LLVM'

