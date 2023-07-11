def accept(path):
    return '/result/' in path


def process(path, addPkg):
    addPkg('CppResult', None, None, None)
    return 'CppResult'

