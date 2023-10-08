#include "transport_catalogue.h"

using namespace std;

namespace catalogue {

void TransportCatalogue::AddStop(const Stop&& stop) {
    stops_.push_back(move(stop));
    accses_to_stops_[string_view(stops_.back().name_stop_)] = &stops_.back();
}

const std::deque<Stop>& TransportCatalogue::GetStops() const {
    return stops_;
}

const Stop* TransportCatalogue::FindStop(std::string_view name_stop) const {
    if (auto it_stop = accses_to_stops_.find(name_stop); it_stop != accses_to_stops_.end()) {
        return it_stop->second;
    }
    return nullptr;
}

void TransportCatalogue::AddBus(const Bus&& bus) {
    buses_.push_back(move(bus));
    accses_to_buses_[string_view(buses_.back().name_bus_)] = &buses_.back();
}

const std::deque<Bus>& TransportCatalogue::GetBuses() const {
    return buses_;
}

const Bus* TransportCatalogue::FindBus(std::string_view name_bus) const {
    if (auto it_bus = accses_to_buses_.find(name_bus); it_bus != accses_to_buses_.end()) {
        return it_bus->second;
    }
    return nullptr;
}

void TransportCatalogue::AddPassingBuses(std::string_view stop) {
    stop_to_passing_buses_[stop];
}

void TransportCatalogue::AddPassingBuses(string_view stop, string_view bus) {
    stop_to_passing_buses_[stop].insert(bus);
}

set<string_view>* TransportCatalogue::GetPassingBuses(std::string_view stop) {
    if (auto it_stop = stop_to_passing_buses_.find(stop); it_stop != stop_to_passing_buses_.end()) {
        return &it_stop->second;
    }
    return nullptr;
}

void TransportCatalogue::SetDistance(string_view from, string_view to, int distance) {
    pair<string_view, string_view> couple_stops = {from, to};
    auto element = stopping_distance_.find(couple_stops);
    if (element != stopping_distance_.end()) {
        element->second = distance;
    } else {
        stopping_distance_[couple_stops] = distance;
        if (to != from) {
            stopping_distance_[{to, from}] = distance;
        } 
    }
}

optional<int> TransportCatalogue::FindDistance(string_view from, string_view to) const {
    if (auto it_dist = stopping_distance_.find({from, to}); it_dist != stopping_distance_.end()) {
        return it_dist->second;
    }
    return nullopt;
}

const std::unordered_map<std::pair<std::string_view, std::string_view>, int, TransportCatalogue::Hasher>& TransportCatalogue::GetMapDistance() const {
    return stopping_distance_;
}

void TransportCatalogue::ComputeLength(int* length_bus, double* geo_length_bus, vector<const Stop*> stops) {
    for (auto it = stops.begin(); it < stops.end(); ++it) {
        if (it + 1 < stops.end()) {
            *geo_length_bus += ComputeDistance(geo::Coordinates{(*it)->coordinates_.lat, (*it)->coordinates_.lng}, geo::Coordinates{(*(it + 1))->coordinates_.lat, (*(it + 1))->coordinates_.lng});
            auto dist = FindDistance((*it)->name_stop_, (*(it + 1))->name_stop_);
            if (dist) {
                *length_bus += *dist;
            }
        }
    }
}

const unordered_map<string_view, Bus*>& TransportCatalogue::GetMapBuses() const {
    return accses_to_buses_;
}

InfoBus TransportCatalogue::GetBusInfo(string_view bus_view) {
    const Bus* bus = FindBus(bus_view);
    if (bus == nullptr) {
        return InfoBus{-1, -1, -1, -1};
    }

    InfoBus info;
    info.count_stops_ = bus->ptr_stops_.size();

    unordered_set<string_view> unique_stops;
    for (auto& stop : bus->ptr_stops_) {
        unique_stops.insert(string_view(stop->name_stop_));
    }
    info.count_unique_stops_ = unique_stops.size(); 

    int length_bus = 0;
    double geo_length_bus = 0;
    ComputeLength(&length_bus, &geo_length_bus, bus->ptr_stops_);
    info.length_bus_ = length_bus;
    info.curvature_ = (double)length_bus / geo_length_bus;
    
    return info;
}

} // namespace catalogue 