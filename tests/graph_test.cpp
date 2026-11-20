#include <gtest/gtest.h>
#include <limits>
#include <string>

#include "graph.hpp"
#include "../src/graph_exceptions.hpp"
#include "../AVL_Tree/avl_tree.hpp"
#include "../libs/lab_3/Pair.hpp"
#include "../libs/Option.hpp"
#include "../libs/lab_3/Pair.hpp"


using NumGraph = Graph<int, std::string, double, AVL_Tree>;
using IntGraph = Graph<int, int, int, AVL_Tree>;
using StringGraph = Graph<int, std::string, std::string, AVL_Tree>;

TEST(GraphPathToString, ValidPathReturnsConcatenatedWeights) {
    StringGraph g;
    g.add_vertex(1, "");
    g.add_vertex(2, "");
    g.add_vertex(3, "");
    g.add_edge(1, 2, "a");
    g.add_edge(2, 3, "b");

    ArraySequence<int> path;
    path.Append(1);
    path.Append(2);
    path.Append(3);

    EXPECT_EQ(g.get_string_from_path(path), "ab");
}

TEST(GraphPathToString, EmptyPathReturnsEmptyString) {
    StringGraph g;
    g.add_vertex(1, "");

    ArraySequence<int> path;

    EXPECT_EQ(g.get_string_from_path(path), "");
}

TEST(GraphPathToString, MissingEdgeReturnsEmptyString) {
    StringGraph g;
    g.add_vertex(1, "");
    g.add_vertex(2, "");
   
    ArraySequence<int> path;
    path.Append(1);
    path.Append(2);

    EXPECT_EQ(g.get_string_from_path(path), "");
}




TEST(GraphVertex, AddVertexIncreasesCount) {
    NumGraph g;
    EXPECT_EQ(g.get_count_vertex(), 0u);
    g.add_vertex(1, "one");
    EXPECT_EQ(g.get_count_vertex(), 1u);
}

TEST(GraphVertex, AddMultipleVertices) {
    NumGraph g;
    g.add_vertex(1, "one");
    g.add_vertex(2, "two");
    g.add_vertex(3, "three");
    EXPECT_EQ(g.get_count_vertex(), 3u);
    EXPECT_TRUE(g.contains_vetex(1));
    EXPECT_TRUE(g.contains_vetex(2));
    EXPECT_TRUE(g.contains_vetex(3));
}

TEST(GraphVertex, AddDuplicateVertexIsIgnored) {
    NumGraph g;
    g.add_vertex(1, "one");
    g.add_vertex(1, "ONE");     
    EXPECT_EQ(g.get_count_vertex(), 1u);
    EXPECT_EQ(g.get_vertex_data(1), "one"); 
}

TEST(GraphVertex, ContainsVertexReturnsFalseForMissing) {
    NumGraph g;
    g.add_vertex(1, "one");
    EXPECT_FALSE(g.contains_vetex(2));
}

TEST(GraphVertex, GetVertexData) {
    NumGraph g;
    g.add_vertex(1, "hello");
    EXPECT_EQ(g.get_vertex_data(1), "hello");
}

TEST(GraphVertex, GetRefVertexDataAllowsModification) {
    NumGraph g;
    g.add_vertex(1, "old");
    g.get_ref_vertex_data(1) = "new";
    EXPECT_EQ(g.get_vertex_data(1), "new");
}

TEST(GraphVertex, GetVerticesReturnsAllKeys) {
    NumGraph g;
    g.add_vertex(10, "a");
    g.add_vertex(20, "b");
    g.add_vertex(30, "c");
    auto keys = g.get_vertices();
    EXPECT_EQ(keys.GetLength(), 3u);
}




TEST(GraphEdge, AddEdgeBetweenExistingVertices) {
    NumGraph g;
    g.add_vertex(1, "one");
    g.add_vertex(2, "two");
    g.add_edge(1, 2, 3.5);
    EXPECT_EQ(g.get_count_edges(), 1u);
    EXPECT_TRUE(g.contains_edge(1, 2));
}

TEST(GraphEdge, AddEdgeIgnoredIfVertexMissing) {
    NumGraph g;
    g.add_vertex(1, "one");
    g.add_edge(1, 2, 3.5);        
    EXPECT_EQ(g.get_count_edges(), 0u);
}

TEST(GraphEdge, AddDuplicateEdgeIsIgnored) {
    NumGraph g;
    g.add_vertex(1, "one");
    g.add_vertex(2, "two");
    g.add_edge(1, 2, 3.5);
    g.add_edge(1, 2, 9.9);           
    EXPECT_EQ(g.get_count_edges(), 1u);
    EXPECT_DOUBLE_EQ(g.get_weight_edge(1, 2), 3.5);
}

TEST(GraphEdge, DirectedEdgeDoesNotExistInReverse) {
    NumGraph g;
    g.add_vertex(1, "one");
    g.add_vertex(2, "two");
    g.add_edge(1, 2, 3.5);
    EXPECT_TRUE(g.contains_edge(1, 2));
    EXPECT_FALSE(g.contains_edge(2, 1)); 
}

TEST(GraphEdge, GetWeight) {
    NumGraph g;
    g.add_vertex(1, "one");
    g.add_vertex(2, "two");
    g.add_edge(1, 2, 7.25);
    EXPECT_DOUBLE_EQ(g.get_weight_edge(1, 2), 7.25);
}

TEST(GraphEdge, RewritingWeight) {
    NumGraph g;
    g.add_vertex(1, "one");
    g.add_vertex(2, "two");
    g.add_edge(1, 2, 3.5);
    g.rewriting_weihgt(1, 2, 9.9);
    EXPECT_DOUBLE_EQ(g.get_weight_edge(1, 2), 9.9);
}

TEST(GraphEdge, GetRefWeightAllowsModification) {
    NumGraph g;
    g.add_vertex(1, "one");
    g.add_vertex(2, "two");
    g.add_edge(1, 2, 3.5);
    g.get_ref_weight_edge(1, 2) = 42.0;
    EXPECT_DOUBLE_EQ(g.get_weight_edge(1, 2), 42.0);
}

TEST(GraphEdge, DeleteEdge) {
    NumGraph g;
    g.add_vertex(1, "one");
    g.add_vertex(2, "two");
    g.add_edge(1, 2, 3.5);
    g.delete_edge(1, 2);
    EXPECT_EQ(g.get_count_edges(), 0u);
    EXPECT_FALSE(g.contains_edge(1, 2));
}

TEST(GraphEdge, DeleteMissingEdgeIsNoop) {
    NumGraph g;
    g.add_vertex(1, "one");
    g.add_vertex(2, "two");
    EXPECT_NO_THROW(g.delete_edge(1, 2));
    EXPECT_EQ(g.get_count_edges(), 0u);
}

TEST(GraphEdge, GetEdgesReturnsAllKeys) {
    NumGraph g;
    g.add_vertex(1, "a");
    g.add_vertex(2, "b");
    g.add_vertex(3, "c");
    g.add_edge(1, 2, 1.0);
    g.add_edge(2, 3, 2.0);
    g.add_edge(1, 3, 3.0);
    EXPECT_EQ(g.get_edges().GetLength(), 3u);
}

TEST(GraphEdge, GetExtentVertexCountsIncidentEdges) {
    NumGraph g;
    g.add_vertex(1, "a");
    g.add_vertex(2, "b");
    g.add_vertex(3, "c");
    g.add_edge(1, 2, 1.0);   
    g.add_edge(1, 3, 2.0); 
    g.add_edge(2, 3, 3.0);  
    EXPECT_EQ(g.get_extent_vertex(1), 2u);
    EXPECT_EQ(g.get_extent_vertex(2), 2u);
    EXPECT_EQ(g.get_extent_vertex(3), 2u);
}





TEST(GraphVertexDelete, DeleteVertexRemovesIt) {
    NumGraph g;
    g.add_vertex(1, "one");
    g.delete_vertex(1);
    EXPECT_FALSE(g.contains_vetex(1));
    EXPECT_EQ(g.get_count_vertex(), 0u);
}

TEST(GraphVertexDelete, DeleteVertexRemovesIncidentEdges) {
    NumGraph g;
    g.add_vertex(1, "a");
    g.add_vertex(2, "b");
    g.add_vertex(3, "c");
    g.add_edge(1, 2, 1.0);
    g.add_edge(2, 1, 2.0);
    g.add_edge(2, 3, 3.0);
    g.add_edge(3, 1, 4.0);

    g.delete_vertex(2);

    EXPECT_FALSE(g.contains_vetex(2));
    EXPECT_FALSE(g.contains_edge(1, 2));
    EXPECT_FALSE(g.contains_edge(2, 1));
    EXPECT_FALSE(g.contains_edge(2, 3));
    EXPECT_TRUE(g.contains_edge(3, 1)); 
    EXPECT_EQ(g.get_count_edges(), 1u);
}

TEST(GraphVertexDelete, DeleteMissingVertexIsNoop) {
    NumGraph g;
    g.add_vertex(1, "one");
    EXPECT_NO_THROW(g.delete_vertex(99));
    EXPECT_EQ(g.get_count_vertex(), 1u);
}





TEST(GraphHelper, ContainsVertexInSequence) {
    NumGraph g;
    ArraySequence<int> seq;
    seq.Append(1);
    seq.Append(2);
    seq.Append(3);
    EXPECT_TRUE(g.contains_vertex(seq, 2));
    EXPECT_FALSE(g.contains_vertex(seq, 99));
}


TEST(GraphDijkstra, SimpleLineGraph) {
    IntGraph g;
    g.add_vertex(1, 0);
    g.add_vertex(2, 0);
    g.add_vertex(3, 0);
    g.add_edge(1, 2, 1);
    g.add_edge(2, 3, 2);

    ArraySequence<int> prev;
    auto dist = g.dijkstra(1, prev);

    auto vertices = g.get_vertices();
    ASSERT_EQ(dist.GetLength(), vertices.GetLength());

    for (std::size_t i = 0; i < vertices.GetLength(); ++i) {
        if (vertices[i] == 1) EXPECT_EQ(dist[i], 0);
        if (vertices[i] == 2) EXPECT_EQ(dist[i], 1);
        if (vertices[i] == 3) EXPECT_EQ(dist[i], 3);
    }
}

TEST(GraphDijkstra, ChoosesShorterPath) {
    IntGraph g;
    g.add_vertex(1, 0);
    g.add_vertex(2, 0);
    g.add_vertex(3, 0);
    g.add_edge(1, 2, 10);
    g.add_edge(1, 3, 1);
    g.add_edge(3, 2, 1);

    ArraySequence<int> prev;
    auto dist = g.dijkstra(1, prev);

    auto vertices = g.get_vertices();
    for (std::size_t i = 0; i < vertices.GetLength(); ++i) {
        if (vertices[i] == 2) EXPECT_EQ(dist[i], 2);  
    }
}

TEST(GraphDijkstra, UnreachableVertexKeepsInfinity) {
    IntGraph g;
    g.add_vertex(1, 0);
    g.add_vertex(2, 0);
    g.add_vertex(3, 0);
    g.add_edge(1, 2, 5);

    ArraySequence<int> prev;
    auto dist = g.dijkstra(1, prev);

    auto vertices = g.get_vertices();
    for (std::size_t i = 0; i < vertices.GetLength(); ++i) {
        if (vertices[i] == 3)
            EXPECT_EQ(dist[i], std::numeric_limits<int>::max());
    }
}

TEST(GraphDijkstra, ThrowsOnMissingStartVertex) {
    IntGraph g;
    g.add_vertex(1, 0);

    ArraySequence<int> prev;
    EXPECT_THROW(g.dijkstra(99, prev), vertex_not_found);
}

TEST(GraphDijkstra, PreviousArrayReconstructsPath) {
    IntGraph g;
    g.add_vertex(1, 0);
    g.add_vertex(2, 0);
    g.add_vertex(3, 0);
    g.add_edge(1, 2, 1);
    g.add_edge(2, 3, 1);

    ArraySequence<int> prev;
    g.dijkstra(1, prev);

    auto vertices = g.get_vertices();
    for (std::size_t i = 0; i < vertices.GetLength(); ++i) {
        if (vertices[i] == 1) EXPECT_EQ(prev[i], 1);
        if (vertices[i] == 2) EXPECT_EQ(prev[i], 1);
        if (vertices[i] == 3) EXPECT_EQ(prev[i], 2);
    }
}

TEST(GraphDijkstra, SingleVertexGraph) {
    IntGraph g;
    g.add_vertex(1, 0);

    ArraySequence<int> prev;
    auto dist = g.dijkstra(1, prev);

    ASSERT_EQ(dist.GetLength(), 1u);
    EXPECT_EQ(dist[0], 0);
}


TEST(GraphDijkstraDouble, WeightedPath) {
    Graph<int, std::string, double, AVL_Tree> g;
    g.add_vertex(1, "a");
    g.add_vertex(2, "b");
    g.add_vertex(3, "c");
    g.add_edge(1, 2, 0.5);
    g.add_edge(2, 3, 0.25);

    ArraySequence<int> prev;
    auto dist = g.dijkstra(1, prev);

    auto vertices = g.get_vertices();
    for (std::size_t i = 0; i < vertices.GetLength(); ++i) {
        if (vertices[i] == 1) EXPECT_DOUBLE_EQ(dist[i], 0.0);
        if (vertices[i] == 2) EXPECT_DOUBLE_EQ(dist[i], 0.5);
        if (vertices[i] == 3) EXPECT_DOUBLE_EQ(dist[i], 0.75);
    }
}