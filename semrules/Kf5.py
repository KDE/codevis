import re

# Order matters, please avoid changing the order of this list
PKG_PATH_MAPPING = {
    'KNewStuff': 'KNewStuff',
    'Attica': 'Attica',
    'KConfigWidgets': 'KConfigWidgets',
    'KIOFileWidgets': 'KIO',
    'KSyntaxHighlighting': 'KSyntaxHighlighting',
    'KActivities': 'KActivities',
    'KCoreAddons': 'KCoreAddons',
    'KIOGui': 'KIO',
    'KTextEditor': 'KTextEditor',
    'KArchive': 'KArchive',
    'KCrash': 'KCrash',
    'KIOWidgets': 'KIO',
    'KTextWidgets': 'KTextWidgets',
    'KAuth': 'KAuth',
    'KDBusAddons': 'KDBusAddons',
    'Kirigami2': 'Kirigami2',
    'KWallet': 'KWallet',
    'KAuthCore': 'KAuthCore',
    'KDocTools': 'KDocTools',
    'KItemViews': 'KItemViews',
    'KWidgetsAddons': 'KWidgetsAddons',
    'KAuthWidgets': 'KAuthWidgets',
    'KGlobalAccel': 'KGlobalAccel',
    'KJobWidgets': 'KJobWidgets',
    'KWindowSystem': 'KWindowSystem',
    'KBookmarks': 'KBookmarks',
    'KGuiAddons': 'KGuiAddons',
    'KMoreTools': 'KNewStuff',
    'KXmlGui': 'KXmlGui',
    'KCodecs': 'KCodecs',
    'KI18n': 'KI18n',
    'KNewStuff3': 'KNewStuff',
    'Solid': 'Solid',
    'KCompletion': 'KCompletion',
    'KI18nLocaleData': 'KI18n',
    'KNotifyConfig': 'KNotifyConfig',
    'KNotifications': 'KNotifications',
    'Sonnet': 'Sonnet',
    'KConfig': 'KConfig',
    'KIconThemes': 'KIconThemes',
    'KPackage': 'KPackage',
    'SonnetCore': 'Sonnet',
    'KConfigCore': 'KConfig',
    'kio': 'KIO',
    'KParts': 'KParts',
    'SonnetUi': 'Sonnet',
    'KConfigGui': 'KConfig',
    'KIO': 'KIO',
    'KPty': 'KPty',
    'Syndication': 'Syndication',
    'KConfigQml': 'KConfig',
    'KIOCore': 'KIO',
    'KService': 'KService',
    'KUserFeedback': 'KUserFeedback',
    'KDNSSD': 'KDNSSD',
    'KItemModels': 'KItemModels',
    'KFileMetaData': 'KFileMetaData',
    'KDeclarative': 'KDeclarative',
    'KCMUtils': 'KCMUtils',
    'purpose': 'purpose',
    'threadweaver': 'threadweaver',
}

def accept(path):
    for pathPrefix in PKG_PATH_MAPPING.keys():
        if f'/{pathPrefix}/'.lower() in path.lower():
            return True
    return False


def process(path, addPkg):
    path = path.lower()
    pkgName = topLevelPackageName(path)
    addPkg(pkgName, None, 'KF5', None)

    if 'build' in path:
        pkgName = processBuildDir(path, pkgName, addPkg)
    elif 'src' in path:
        pkgName = processSourceDir(path, pkgName, addPkg)

    if 'kxmlguibuilder' in path:
        print(f'kxmlguibuilder pkgName={pkgName}')
    return pkgName


def topLevelPackageName(path):
    for pathPrefix, pkgName in PKG_PATH_MAPPING.items():
        if f'/{pathPrefix}/'.lower() in path:
            return pkgName
    return 'Unknown'


def processSourceDir(path, pkgName, addPkg):
    for innerPkg in re.split('src', path)[-1].split('/')[1:-1]:
        addPkg(innerPkg, pkgName, None, None)
        pkgName = innerPkg
    return pkgName


def processBuildDir(path, pkgName, addPkg):
    if 'src/lib' in path:
        # Exported headers
        addPkg(f'{pkgName}/ForwardingHeaders', pkgName, None, None)
        pkgName = f'{pkgName}/ForwardingHeaders'
    else:
        if 'kxmlguibuilder' in path:
            print(f'kxmlguibuilder pkgName={pkgName}')
        addPkg(f'{pkgName}/BuildGenFiles', pkgName, None, None)
        pkgName = f'{pkgName}/BuildGenFiles'
    return pkgName

