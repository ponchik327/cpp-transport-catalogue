#include "transport_router.h"

using namespace std;

TransportRouter::TransportRouter(const catalogue::TransportCatalogue& t_c, const RouteSettings& rs) 
    : tran_catal_(t_c)
    , graph_(move(InitialGraph(rs))) {
}

std::tuple<std::optional<TransportRouter::RouteInfo>, const TransportRouter::Graph&> TransportRouter::GetRouteInfo(std::string_view from, std::string_view to) const {
    const static graph::Router router{graph_};
    size_t id_from = tran_catal_.FindStop(from)->id_ * 2;
    size_t id_to = tran_catal_.FindStop(to)->id_ * 2;
    auto info = router.BuildRoute(id_from, id_to);
    return {info, graph_};
}

void TransportRouter::ComputeEdgesDirectBus(TransportRouter::Graph& grph, Bus* const ptr_bus, double bus_velocity) {
    const auto& stops = ptr_bus->ptr_stops_;
    for (size_t i_from = 0; i_from < stops.size(); ++i_from) { 
        int span_count_forward = 1;
        int span_count_backward = 1;
        double total_dist_forward = 0;
        double total_dist_backward = 0;
        for (size_t i_to = i_from + 1; i_to < stops.size(); ++i_to) { 
            size_t id_from = tran_catal_.FindStop(stops[i_from]->name_stop_)->id_ * 2;
            size_t id_to = tran_catal_.FindStop(stops[i_to]->name_stop_)->id_ * 2;
            
            auto dist_forward = tran_catal_.FindDistance(stops[i_to - 1]->name_stop_, stops[i_to]->name_stop_);
            if (dist_forward) {
                total_dist_forward += *dist_forward;
                graph::Edge<RouteWeight> edge{id_from + 1, id_to, RouteWeight{(total_dist_forward / 1000) / (bus_velocity / 60), ptr_bus->name_bus_, span_count_forward++, true}};
                grph.AddEdge(edge);
            }

            auto dist_backward = tran_catal_.FindDistance(stops[i_to]->name_stop_, stops[i_to - 1]->name_stop_);
            if (dist_backward) {
                total_dist_backward += *dist_backward;
                graph::Edge<RouteWeight> edge{id_to + 1, id_from, RouteWeight{(total_dist_backward / 1000) / (bus_velocity / 60), ptr_bus->name_bus_, span_count_backward++, true}};
                grph.AddEdge(edge);
            }
        }
    }
}

void TransportRouter::ComputeEdgesCircularBus(TransportRouter::Graph& grph, Bus* const ptr_bus, double bus_velocity) {
    const auto& stops = ptr_bus->ptr_stops_;
    for (size_t i_from = 0; i_from < stops.size(); ++i_from) {
        int span_count = 1;
        double total_dist = 0;
        for (size_t i_to = i_from + 1; i_to < stops.size(); ++i_to) { 
            auto dist = tran_catal_.FindDistance(stops[i_to - 1]->name_stop_, stops[i_to]->name_stop_);
            if (dist) {
                total_dist += *dist;
                size_t id_from = tran_catal_.FindStop(stops[i_from]->name_stop_)->id_ * 2 + 1;
                size_t id_to = tran_catal_.FindStop(stops[i_to]->name_stop_)->id_ * 2;
                graph::Edge<RouteWeight> edge{id_from, id_to, RouteWeight{(total_dist / 1000) / (bus_velocity / 60), ptr_bus->name_bus_, span_count++, true}};
                grph.AddEdge(edge);
            }
        }
    }
}

TransportRouter::Graph TransportRouter::InitialGraph(const RouteSettings& rs) {
    size_t count_stops = tran_catal_.GetStops().size();
    graph::DirectedWeightedGraph<RouteWeight> grph{count_stops * 2};
    for (int i = 0; i < count_stops; ++i) {
        size_t id_from = i * 2;
        size_t id_to = i * 2 + 1;
        grph.AddEdge(graph::Edge<RouteWeight>{id_from, id_to, RouteWeight{rs.bus_wait_time, (tran_catal_.GetStops().begin() + i)->name_stop_, 0, false}});
    }
    for (const auto& [name, ptr_bus] : tran_catal_.GetMapBuses()) {
        if (ptr_bus->type_ == Bus::BusType::CIRCULAR) {
            ComputeEdgesCircularBus(grph, ptr_bus, rs.bus_velocity);
        } else if (ptr_bus->type_ == Bus::BusType::DIRECT) {
            ComputeEdgesDirectBus(grph, ptr_bus, rs.bus_velocity);
        }
    }
    return grph;
}