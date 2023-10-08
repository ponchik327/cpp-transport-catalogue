#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <optional>

#include "geo.h"
#include "domain.h"

namespace catalogue {

class TransportCatalogue {

private:

    struct Hasher {
        size_t operator() (const std::pair<std::string_view, std::string_view>& key) const {
            return hasher(key.first) * 34 + hasher(key.second) * (34 * 34);
        }
        std::hash<std::string_view> hasher;
    };

public:
    void AddStop(const Stop&& stop);
    const std::deque<Stop>& GetStops() const;
    const Stop* FindStop(std::string_view name_stop) const;

    void AddBus(const Bus&& bus);
    const std::deque<Bus>& GetBuses() const;
    const Bus* FindBus(std::string_view name_bus) const;

    const std::unordered_map<std::string_view, Bus*>& GetMapBuses() const;
    InfoBus GetBusInfo(std::string_view bus_view);

    void AddPassingBuses(std::string_view stop);
    void AddPassingBuses(std::string_view stop, std::string_view bus);
    std::set<std::string_view>* GetPassingBuses(std::string_view stop);

    void SetDistance(std::string_view from, std::string_view to, int distance);
    std::optional<int> FindDistance(std::string_view from, std::string_view to) const;
    const std::unordered_map<std::pair<std::string_view, std::string_view>, int, Hasher>& GetMapDistance() const;

private :

    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, Stop*> accses_to_stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, Bus*> accses_to_buses_;
    std::unordered_map<std::pair<std::string_view, std::string_view>, int, Hasher> stopping_distance_;
    std::unordered_map<std::string_view, std::set<std::string_view>> stop_to_passing_buses_;

    void ComputeLength(int* length_bus, double* geo_length_bus, std::vector<const Stop*> stops);

};

} // namespace catalogue