def accept(path):
    import re
    return re.search('/llvm(-?[0-9]+)?/', path) is not None or re.search('/llvm-c(-?[0-9]+)?/', path) is not None


def process(path, addPkg):
    import re

    addPkg('LLVM', None, None, None)

    m = re.search('/(llvm-.*?)/', path)
    if not m or not m[0]:
        return 'LLVM'

    prev = 'LLVM'
    nestedPkgs = path.split(m[0])[1].split("/")[:-1]
    for pkg in nestedPkgs:
        addPkg(prev + '/' + pkg, prev, None, None)
        prev = prev + '/' + pkg
    return prev
