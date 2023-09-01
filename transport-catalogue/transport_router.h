#pragma once
#include <string_view>
#include <optional>
#include <tuple>

#include "domain.h"
#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"

struct RouteSettings {
    double bus_wait_time = -1;
    double bus_velocity = -1;
};

class TransportRouter {
private:
    struct RouteWeight {
        double weight;
        std::string_view name;
        int span_count;
        bool is_bus;

        bool operator< (const RouteWeight& other) const {
            return weight < other.weight;
        }

        bool operator> (const RouteWeight& other) const {
            return weight > other.weight;
        }

        RouteWeight operator+ (const RouteWeight& other) const {
            return {weight + other.weight, name, span_count, is_bus};
        }
    };

public:
    TransportRouter(const catalogue::TransportCatalogue& t_c, const RouteSettings& rs);
    
    using Graph = graph::DirectedWeightedGraph<RouteWeight>;
    using RouteInfo = graph::Router<RouteWeight>::RouteInfo;
    std::tuple<std::optional<RouteInfo>, const Graph&> GetRouteInfo(std::string_view from, std::string_view to) const;

private:
    const catalogue::TransportCatalogue& tran_catal_;
    Graph graph_;

    Graph InitialGraph(const RouteSettings& rs);
    void ComputeEdgesDirectBus(Graph& grph, Bus* const ptr_bus, double bus_velocity);
    void ComputeEdgesCircularBus(Graph& grph, Bus* const ptr_bus, double bus_velocity);

};