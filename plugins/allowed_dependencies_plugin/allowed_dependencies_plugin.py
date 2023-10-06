import pyLksPlugin as plg
import os


def hookMainNodeChanged(handler):
    source_path = handler.getProjectData().getSourceCodePath()

    for entity in handler.getVisibleEntities():
        if entity.getType() != plg.EntityType.Package:
            continue

        parent = entity.getParent()
        if not parent:
            continue

        prefix = f"{source_path}/groups/{parent.getName()}"
        depfile = f"{prefix}/{entity.getName()}/package/{entity.getName()}.dep"
        if not os.path.isfile(depfile):
            continue
        with open(depfile, 'r') as f:
            allowed_dependencies = [d.strip() for d in f.readlines()]

        repaintEdgesGivenAllowedDependencies(handler, entity, allowed_dependencies)


def repaintEdgesGivenAllowedDependencies(handler, entity, allowed_dependencies):
    for dependency in entity.getDependencies():
        edge = handler.getEdgeByQualifiedName(entity.getQualifiedName(), dependency.getQualifiedName())
        if dependency.getName() in allowed_dependencies:
            edge.setColor(plg.Color(10, 10, 200))
        else:
            edge.setColor(plg.Color(200, 40, 40))
