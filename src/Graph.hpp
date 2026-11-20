#pragma once
#include <concepts>
#include <type_traits> 
#include "../AVL_Tree/idictionary.hpp"
#include "../libs/lab_3/Pair.hpp"
#include "graph_exceptions.hpp"
#include "../libs/Option.hpp"
#include "../libs/lab_3/Pair.hpp"


template<typename T, typename K, typename V>
concept dictionary = std::derived_from<T, IDdictionary<K, V>>;

template<typename T>
concept Num = std::is_arithmetic_v<T>;

template<typename Index,
    typename VertexData,
    typename Weight,
    template<typename, typename> class ContainerOne
>
requires 
    dictionary<ContainerOne<Index, VertexData>, 
                    Index, VertexData>
    &&
    dictionary<ContainerOne<Pair<Index, Index>, Weight>,
                    Pair<Index, Index>, Weight>

class Graph{
public: 
    using EdgeKey = Pair<Index, Index>;
private:
    ContainerOne<Index, VertexData> m_vertexs;
    ContainerOne<EdgeKey, Weight> m_edges; 

    void relax_edge(const Index& current,const Index& neighbor, ArraySequence<Weight>& dist,
                    const ArraySequence<Index>& vertices,
                    ArraySequence<Index>& previous){
        
        Option<std::size_t> current_pos = find_position(current, vertices);
        Option<std::size_t> neighbor_pos = find_position(neighbor, vertices);

        if (!current_pos.HasValue() || !neighbor_pos.HasValue()) {
            return;
        }

        std::size_t current_index = current_pos.GetValue();
        std::size_t neighbor_index = neighbor_pos.GetValue();       

        if (dist[current_index] == std::numeric_limits<Weight>::max()) {
            return;
        }

        Weight weight = get_weight_edge(current, neighbor);
        Weight new_dist = dist[current_index] + weight;

        if(new_dist  < dist[neighbor_index]){
            dist[neighbor_index] = new_dist;
            previous[neighbor_index] = current;
        }

    }

    Option<std::size_t> find_position(const Index& value, const ArraySequence<Index>& seq) const {
        for (std::size_t i = 0; i < seq.GetLength(); ++i) {
            if (seq[i] == value) {
                return Option<std::size_t>::Some(i);
            }
        }
        return Option<std::size_t>::None();
    }
public:

    void add_vertex(const Index& index, const VertexData& vertex_data){
        if(m_vertexs.contains_key(index)){
            return;
        }
        m_vertexs.add(index, vertex_data);
    }

    void add_edge(const Index& from, const Index& to, const Weight& weight){
        if (!m_vertexs.contains_key(from) ||
            !m_vertexs.contains_key(to)) {
            return;
        }
        EdgeKey key(from, to);
        if(m_edges.contains_key(key)){
            return;
        }
        m_edges.add(key, weight);
    }

    ArraySequence<Index> get_vertices() const { 
        return m_vertexs.get_keys(); 
    }
    ArraySequence<EdgeKey> get_edges() const {
        return m_edges.get_keys();
    }

    void rewriting_weihgt(const Index& from, const Index& to, const Weight& weight){
        EdgeKey key(from, to);
       m_edges.get_ref_by_key(key) = weight;
    }

    void delete_edge(const Index& from, const Index& to){
        EdgeKey key(from, to);
        if(!m_edges.contains_key(key)){
            return;
        }
        m_edges.remove_by_key(key);
    }

    void delete_vertex(const Index& index){
        if(!m_vertexs.contains_key(index)){
            return;
        }
        ArraySequence<EdgeKey> edge_keys = m_edges.get_keys(); 
        for(const auto& key: edge_keys) {
            if(key.GetFirst() == index || key.GetSecond() == index){
                m_edges.remove_by_key(key);
            }
        }
        m_vertexs.remove_by_key(index);
    }

    bool contains_vetex(const Index& index) const {
        return m_vertexs.contains_key(index);
    }

    bool contains_edge(const Index& from, const Index& to) const {
        return m_edges.contains_key(EdgeKey(from, to));
    }

    VertexData get_vertex_data(const Index& index)const {
        return m_vertexs.get_elem_by_key(index);
    }

    VertexData& get_ref_vertex_data(const Index& index){
        return m_vertexs.get_ref_by_key(index);
    }

    Weight get_weight_edge(const Index& from, const Index& to) const{
        EdgeKey key(from, to);
        return m_edges.get_elem_by_key(key);
    }

    Weight& get_ref_weight_edge(const Index& from, const Index& to){
        EdgeKey key(from, to);
        return m_edges.get_ref_by_key(key);
    }

    unsigned int get_count_vertex() const{
        return m_vertexs.get_count();
    }

    unsigned int get_count_edges() const {
        return m_edges.get_count();
    }


    bool contains_vertex(const ArraySequence<Index>& sequence, const Index& vertex) const {
        for (const auto& item : sequence) {
            if (item == vertex) {
                return true;
            }
        }
        return false;
    }

    unsigned int get_extent_vertex(const Index& index){
        unsigned int extent = 0;
        auto keys = m_edges.get_keys();
        for (const auto& key: keys){
            if(key.GetFirst() == index || key.GetSecond() == index){
                extent +=1;
            }
        }
        return extent;
    }

   Option<Index> find_min_dist(const ArraySequence<Index>& unvisited, const ArraySequence<Weight>& dist, const ArraySequence<Index>& vertices) const {

        if (unvisited.GetLength() == 0) {
            return Option<Index>::None();
        }

        Weight min_dist = std::numeric_limits<Weight>::max();
        Index min_dist_vertex = unvisited[0];
        bool found = false;

        for (const auto& i : unvisited) {
            Option<std::size_t> index = find_position(i, vertices);

            if (!index.HasValue()) {
                continue;
            }

            Weight dist_vertex = dist[index.GetValue()];

            if (dist_vertex < min_dist) {
                min_dist = dist_vertex;
                min_dist_vertex = i;
                found = true;
            }
        }

        if (!found) {
            return Option<Index>::None();
        }

        return Option<Index>::Some(min_dist_vertex);
}

    void delete_from_sequence(ArraySequence<Index>& sequence, const Index& vertex){
        Option<std::size_t> position = find_position(vertex, sequence);
        if(!position.HasValue()){
            return;
        }
        sequence.RemoveAt(position.GetValue());
    }

    ArraySequence<Weight> dijkstra( const Index& start, ArraySequence<Index>& previous)
        requires Num<Weight>
    {
        ArraySequence<Index> vertices = m_vertexs.get_keys();
        if (!m_vertexs.contains_key(start)) {
            throw vertex_not_found("Начальной вершины не существует");
        }

        auto dist = ArraySequence<Weight>();
        previous = ArraySequence<Index>();

        Weight infinity = std::numeric_limits<Weight>::max();

        for (const auto& vertex : vertices) {
            dist.Append(infinity);
            previous.Append(Index{});
        }

        Option<std::size_t> start_pos_opt = find_position(start, vertices);
        if (!start_pos_opt.HasValue()) {
            throw vertex_not_found("Начальной вершины нет в списке вершин");
        }
        std::size_t start_pos = start_pos_opt.GetValue();
        dist[start_pos] = 0;
        previous[start_pos] = start;
        
        ArraySequence<Index> unvisited = vertices;
        ArraySequence<Index> visited;

        auto edges = m_edges.get_keys();

        while (unvisited.GetLength() > 0) {
            //смотрим вершину из непосещенных до которой можно добраться за минимальный вес
            Option<Index> current_option = find_min_dist(unvisited, dist, vertices);

            if (!current_option.HasValue()) {
                break;
            }

            Index current = current_option.GetValue();

            delete_from_sequence(unvisited, current);
            visited.Append(current);

            for (const auto& edge : edges) {
                if (edge.GetFirst() == current) {
                    Index neighbor = edge.GetSecond();

                    if (contains_vertex(visited, neighbor)) {
                        continue;
                    }

                    relax_edge(current, neighbor, dist, vertices, previous);
                }
            }
        }
        return dist;
    }

    std::string get_string_from_path(const ArraySequence<Index>& path) const 
        requires std::same_as<Weight, std::string>
    {
        std::string result;

        if (path.GetLength() == 0) {
            return result;
        }

        for (std::size_t i = 0; i < path.GetLength(); i++) {
            if (!m_vertexs.contains_key(path[i])) {
                return{};
            }

            if (i + 1 < path.GetLength()) {

                if (!m_edges.contains_key(EdgeKey(path[i], path[i + 1]))) {
                   return{};
                }

                result += get_weight_edge(path[i], path[i + 1]);
            }
        }

        return result;
    }

};

















