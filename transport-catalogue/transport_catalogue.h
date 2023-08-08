#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <set>

#include "geo.h"

namespace catalogue {

struct Stop
{
    std::string name_stop_;
    geo::Coordinates coordinates_;
};

struct Bus
{
    enum class BusType {
        CIRCULAR,
        DIRECT
    };
    
    std::string name_bus_;
    std::vector<Stop*> ptr_stops_;
    BusType type_;
};

struct InfoBus {
    int count_stops_;
    int count_unique_stops_;
    int length_bus_;
    double curvature_;
};

class TransportCatalogue {

public:
    void AddStop(const Stop&& stop);
    Stop* FindStop(std::string_view name_stop);

    void AddBus(const Bus&& bus);
    Bus* FindBus(std::string_view name_bus);
    const std::unordered_map<std::string_view, Bus*>& GetMapBuses() const;
    InfoBus GetBusInfo(std::string_view bus_view);

    void AddPassingBuses(std::string_view stop);
    void AddPassingBuses(std::string_view stop, std::string_view bus);
    std::set<std::string_view>* GetPassingBuses(std::string_view stop);

    void SetDistance(std::string_view from, std::string_view to, int distance);

private :

    struct Hasher {
        size_t operator() (const std::pair<std::string_view, std::string_view>& key) const {
            return hasher(key.first) * 34 + hasher(key.second) * (34 * 34);
        }
        std::hash<std::string_view> hasher;
    };

    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, Stop*> accses_to_stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, Bus*> accses_to_buses_;
    std::unordered_map<std::pair<std::string_view, std::string_view>, int, Hasher> stopping_distance_;
    std::unordered_map<std::string_view, std::set<std::string_view>> stop_to_passing_buses_;

    void ComputeLength(int* length_bus, double* geo_length_bus, std::vector<Stop*> stops);
    int FindDistance(std::string_view from, std::string_view to);

};

} // namespace catalogue