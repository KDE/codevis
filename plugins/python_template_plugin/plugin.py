import types
import pyLksPlugin

def recoulourize(actionHandler):
    pigmentRed = pyLksPlugin.Color(237, 27, 36, 255)
    viewedEntities = actionHandler.getAllEntitiesInCurrentView()
    for viewedEntity in viewedEntities:
        viewedEntity.setColor(pigmentRed)

# The Hook methods are going to be used by codevis
# as entry points to the system. this is one of the
# most simple examples possible, a way to hook
# the graphics view context menu with new features.
# we are adding the recolourize python call when
# the user right clicks on the view, and acesses the menu.
def hookGraphicsViewContextMenu(menuHandler):
    menuHandler.registerContextMenu("Color the elements on the screen", recoulourize)
