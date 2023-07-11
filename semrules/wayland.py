def accept(path):
    return f'/wayland-' in path


def process(path, addPkg):
    addPkg(f'wayland', None, None, None)
    return f'wayland'

