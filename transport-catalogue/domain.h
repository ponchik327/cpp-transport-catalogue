#pragma once
#include <string>
#include <vector>
#include <set>

#include "geo.h"

struct Stop
{
    std::string name_stop_;
    double latitude_;
    double longitude_;
    std::set<std::string_view> passing_buses_;
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