#pragma once

#include "GraphAlgorithms.hpp"

namespace smartresq {

/**
 * @brief Backwards-compatible alias for RouteResult used by legacy callers.
 */
using ShortestPathResult = RouteResult;

/**
 * @brief Router interface delegating to GraphAlgorithms to avoid code duplication.
 */
class DijkstraRouter {
public:
    static ShortestPathResult computeShortestPath(const CityGraph& graph,
                                                  const std::string& start_node,
                                                  const std::string& target_node) {
        return GraphAlgorithms::dijkstra(graph, start_node, target_node);
    }

    static NearestCandidateResult findNearestCandidate(const CityGraph& graph,
                                                        const std::string& origin_node,
                                                        const std::vector<std::string>& candidates) {
        return GraphAlgorithms::findNearestCandidate(graph, origin_node, candidates);
    }
};

} // namespace smartresq
