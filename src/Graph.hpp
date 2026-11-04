#pragma once
#include <concepts>
#include "AVL_Tree/idictionary.hpp"
#include "libs/lab_3/Pair.hpp"

template<typename T> 
concept numerical = std::is_integral_v<T> || std::is_floating_point_v<T>;

template<typename T, typename K, typename V>
concept dictionary = std::derived_from<T, IDdictionary<K, V>>;

template<
    typename Index, 
    template <typename , typename > class ContainerOne, 
    numerical Weight, 
    template<typename> class ContainerTwo
>
requires dictionary<ContainerOne<Index, ContainerTwo<Pair<Index, Weight>>>, 
                    Index, Pair<Index, Weight>>

class Graph{
private:
    ContainerOne<Index, ContainerTwo<Pair<Index, Weight>>> ;
};
