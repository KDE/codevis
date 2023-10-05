import pyLksPlugin as plg
import os


def hookMainNodeChanged(handler):
    SOURCE_PATH = handler.getProjectData().getSourceCodePath()

    for e in handler.getVisibleEntities():
        if e.getType() == plg.EntityType.Package:
            prefix = f"{SOURCE_PATH}/groups/{e.getParent().getName()}"
            filename = f"{prefix}/{e.getName()}/package/{e.getName()}.dep"
            if not os.path.isfile(filename):
                continue

            with open(filename, 'r') as f:
                allowed_dependencies = [d.strip() for d in f.readlines()]
                for dependency in e.getDependencies():
                    edge = handler.getEdgeByQualifiedName(e.getQualifiedName(), dependency.getQualifiedName())
                    if dependency.getName() in allowed_dependencies:
                        edge.setColor(plg.Color(10, 10, 200))
                    else:
                        edge.setColor(plg.Color(200, 40, 40))
