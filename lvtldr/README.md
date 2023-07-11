# lvtldr: Physical Loader

lvtldr houses the code that loads physical graphs from the code database
into the graphics scene. If we get around to modernising the class graph loader,
that would go here too.

## Layout
- lakosiannode: All LakosianNode pointers in lvtldr are owned by GraphStorage
  in this component.
- physicalloader: This component implements the loading policy for physical
  graphs
- graphloader: This component collates all vertices to be added to the graph,
  finds all of the appropriate edges between these vertices, and adds everything
  in a safe (parents before children) order to the lvtshr::IGraphLoader

## Loading Policy
See doc/loading.md
