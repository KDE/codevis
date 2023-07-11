PKG_LIST = [
    'x86_64-linux-gnu',
    'asm-generic',
    'c++',
    'netinet',
    'arpa',
    'net',
    'linux',
    'clang',
    'rpc',
]
FILE_LIST = [
    'alloca.h',
    'assert.h',
    'features.h',
    'stdc-predef.h',
    'stdio.h',
    'stdlib.h',
    'wchar.h',
    'endian.h',
    'pthread.h',
    'sched.h',
    'time.h',
    'strings.h',
    'ctype.h',
    'errno.h',
    'limits.h',
    'stdint.h',
    'string.h',
    'locale.h',
    'wctype.h',
    'math.h',
    'signal.h',
    'termios.h',
    'unistd.h',
    'dirent.h',
    'fcntl.h',
    'grp.h',
    'pwd.h',
    'fcntl.h',
    'libintl.h',
    'zlib.h',
    'zconf.h',
    'poll.h',
    'inttypes.h',
    'netdb.h',
    'crypt.h',
]

def accept(path):
    for pkgName in PKG_LIST:
        if f'/{pkgName}/' in path:
            return True
    for fileName in FILE_LIST:
        if fileName in path:
            return True
    return False


def process(path, addPkg):
    pkgName = "SystemLibraries"
    addPkg(pkgName, None, None, None)
    return pkgName

