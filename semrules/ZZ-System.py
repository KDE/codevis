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
    'unicode',
    'xcb',
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
    'dlfcn.h',
    'libudev.h',
    'semaphore.h',
    'malloc.h',
    'mntent.h',
    'paths.h',
    'setjmp.h',
    'syslog.h',
    'iconv.h',
    'regex.h',
    'pty.h',
    'termio.h',
    'utmp.h',
    'fstab.h',
    'ifaddrs.h',
    'libgen.h',
    'resolv.h',
    'memory.h',
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

