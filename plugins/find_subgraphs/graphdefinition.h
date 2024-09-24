#pragma once

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/biconnected_components.hpp> // articulation_points
#include <boost/graph/connected_components.hpp> // connected_components
#include <boost/graph/copy.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/named_graph.hpp>
#include <boost/graph/properties.hpp>

namespace Codethink::lvtqtc {
class LakosEntity;
}

// Plugin specific data for Boost Graph
struct VertexProps {
    Codethink::lvtqtc::LakosEntity *ptr;
};

using Graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, VertexProps>;

using Vertex = Graph::vertex_descriptor;
using Filtered = boost::filtered_graph<Graph, boost::keep_all, std::function<bool(Vertex)>>;
