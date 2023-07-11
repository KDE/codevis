## Graph Loading Policy
Vertices and edges are considered separately (vertices in physicalloader and
edges in graphloader).

### Vertices
A graph is loaded by visiting each node one by one, starting with the main node
(e.g. if we are drawing a graph of `bal`, `bal` is the main node).

Visiting a node causes it to be added to the graph. We then make policy decisions
about which nodes to visit next after the current node. These fall into three
categories

1. Should we load forward or reverse dependencies of this vertex? This is done
   when the "Load External Dependencies" toggle is enabled, if the vertex is in
   the same package group as the main node or if the vertex is not too distant
   (1) from the main node.
2. Should we load the parent of this vertex? This is done for anything in a
   different package group to the main node (so these package groups can be
   easily collapsed) or if the node is of a smaller type than the main node
   (e.g. the main node is a package and this node is a component).
3. Should we load the children of this vertex? If the node is in a different
   package group to the main node, don't load children to any greater detail
   than components (if we load types as well, graph load times explode). If the
   node is in the same package group as the main node, load children if the node
   is the same type (group, package, etc) as the main node or if the main
   node is a component and this this node is a type (so we don't miss nested
   types in component graphs).

Care is taken so that each node is visited only once, even in the presence of
circular dependencies.

(1) Distance is measured by the number of graph edges between this node and the
    main node. The thresholds are 2 edges for package group and component graphs
    and only 1 edge on package graphs (because these tend to be more congested.

These policy rules can seem convoluted, they walk a very fine line between
omitting necessary detail and creating graphs that are unreadably large and
complex.

#### Manual Intervention
All three kinds of vertex loading policy can be overridden on a per-vertex basis
by right clicking on that vertex in the graph. The right click menu also tells
you what policy decision was made for that node when loading the graph: for
example if it says "Don't load edges" then all of the dependencies were loaded,
if it says "Load edges" then there (possibly) are more dependencies to load for
that vertex.

Sometimes setting a "never load" manual option appears not to do anything. For
example, we might tell a node not to load its children but find some of those
children are still loaded for another reason (e.g. due to edges loaded from
something else's children).

### Edges
Once `lvtldr::PhysicalLoader` has picked out which vertices to include in the
graph, `lvtldr::GraphLoader` will load *all* edges existing between those
vertices from the database.

By default, displayed graphs only render the transitive reduction of dependency
edges. For example, if we have a graph like
```
A -- B -- C
 \_______/
```

The edge between A and C will not be drawn because this edge is redundant
(being already implied by transitivity from A - B - C).

These redundant edges can be displayed by right clicking on empty space and
selecting "show redundant edges".
