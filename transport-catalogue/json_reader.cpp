#include "json_reader.h"

using namespace std;

namespace catalogue {

namespace json_handler {

// ----------- реализация JSON ридера ---------------------

JsonReader::JsonReader(istream& input, TransportCatalogue& t_c) 
    : input_(input) 
    , tran_catal_(t_c)
    , doc_json_(json::Load(input_)) {
        InitialTransportCatalogue(tran_catal_);
        InitialGraph(graph_);
}

void JsonReader::MakeRequestQueue() {
    for (const auto& request : doc_json_.GetRoot().AsMap().at("base_requests"s).AsArray()) {
        string_view type = request.AsMap().at("type"s).AsString();
        if (type == "Stop"sv) {
            request_queue_add_.push_front({move(request.AsMap()), RequestType::STOP});
        }
        if (type == "Bus"sv) {
            request_queue_add_.push_back({move(request.AsMap()), RequestType::BUS});
        }  
    }
}

JsonReader::RequestQueque& JsonReader::GetRequestQueue() {
    return request_queue_add_;
}

void JsonReader::InitialTransportCatalogue(TransportCatalogue& t_c) {
    MakeRequestQueue();
    for (auto& [request, type] : request_queue_add_) {
        if (type == RequestType::STOP) {
            Stop stop = ReadStop(request, t_c);
            t_c.AddStop(move(stop));
        }
        if (type == RequestType::BUS) {
            Bus bus = ReadBus(request, t_c);
            t_c.AddBus(move(bus));
        }
    }
}

// ----------- считывание отсановок Json ридера ---------------------

Stop JsonReader::ReadStop(Request& request, TransportCatalogue& t_c) {
    string_view name_from = request.at("name"s).AsString();
    double lat = request.at("latitude"s).AsDouble();
    double lng = request.at("longitude"s).AsDouble();
    for (const auto& [name, distance] : request.at("road_distances"s).AsMap()) {
        t_c.SetDistance(name_from, name, distance.AsInt());
    }
    t_c.AddPassingBuses(name_from);
    static int id = 0;
    return Stop{(string)name_from, lat, lng, id++};
}

// ----------- считывание маршрутов Json ридера ---------------------

Bus JsonReader::ReadBus(Request& request, TransportCatalogue& t_c) {
    string_view name = request.at("name"s).AsString();
    vector<Stop*> stops;
    json::Array arr = request.at("stops"s).AsArray();
    for (const auto& stop_str : arr) {
        Stop* stop = t_c.FindStop(stop_str.AsString());
        t_c.AddPassingBuses(stop->name_stop_, name);
        stops.push_back(stop);
    }
    Bus::BusType type;
    if (!request.at("is_roundtrip"s).AsBool()) {
        type = Bus::BusType::DIRECT;
        for (auto it = arr.rbegin() + 1; it < arr.rend(); ++it) {
            stops.push_back(t_c.FindStop((*it).AsString()));
        }
    } else {
        type = Bus::BusType::CIRCULAR;
    }
    return Bus{(string)name, move(stops), type};
}

// ----------------- обработка настроек MapRender'a --------------------

uint8_t JsonReader::NodeToUint8(const json::Node& color, int index) {
    return static_cast<uint8_t>(color.AsArray()[index].AsInt());
}

svg::Color JsonReader::InitialColor(const json::Node& color) {
    svg::Color temp_color;
    if (color.IsString()) {
        temp_color = color.AsString();
    } else if (color.AsArray().size() == 3){
        temp_color = svg::Rgb{NodeToUint8(color, 0), NodeToUint8(color, 1), NodeToUint8(color, 2)};
    } else {
        temp_color = svg::Rgba{NodeToUint8(color, 0), NodeToUint8(color, 1), NodeToUint8(color, 2), color.AsArray()[3].AsDouble()};
    }
    return temp_color;
}

visual::MapRender JsonReader::ProcessingRenderSettings() {
    visual::Properties prop;
    svg::Point stop_off, bus_off;
    svg::Color und_col;
    vector<svg::Color> col_palet;
    for (const auto& [name, value] : doc_json_.GetRoot().AsMap().at("render_settings"s).AsMap()) {
        if (value.IsDouble()) {
            prop.insert({name, value.AsDouble()});
        } else {
            if (name == "bus_label_offset"sv) {
                bus_off = svg::Point{value.AsArray()[0].AsDouble(), value.AsArray()[1].AsDouble()};
            }
            if (name == "stop_label_offset"sv) {
                stop_off = svg::Point{value.AsArray()[0].AsDouble(), value.AsArray()[1].AsDouble()};
            }
            if (name == "underlayer_color"sv) {
                und_col = move(InitialColor(value));
            }
            if (name == "color_palette"sv) {
                for (const auto& color : value.AsArray()) {
                    col_palet.push_back(move(InitialColor(color)));
                }
            }
        }
    }
    return {move(prop), bus_off, stop_off, und_col, move(col_palet)};
}

// --------------------------- считывание настроек запроса Route --------------------------------

void JsonReader::ComputeEdgesDirectBus(graph::DirectedWeightedGraph<RouteWeight>& grph, Bus* const bus, double bus_velocity) {
    const auto& stops = bus->ptr_stops_;
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
                graph::Edge<RouteWeight> edge{id_from + 1, id_to, RouteWeight{(total_dist_forward / 1000) / (bus_velocity / 60), bus->name_bus_, span_count_forward++, true}};
                grph.AddEdge(edge);
            }

            auto dist_backward = tran_catal_.FindDistance(stops[i_to]->name_stop_, stops[i_to - 1]->name_stop_);
            if (dist_backward) {
                total_dist_backward += *dist_backward;
                graph::Edge<RouteWeight> edge{id_to + 1, id_from, RouteWeight{(total_dist_backward / 1000) / (bus_velocity / 60), bus->name_bus_, span_count_backward++, true}};
                grph.AddEdge(edge);
            }
        }
    }
}

void JsonReader::ComputeEdgesCircularBus(graph::DirectedWeightedGraph<RouteWeight>& grph, Bus* const bus, double bus_velocity) {
    const auto& stops = bus->ptr_stops_;
    for (size_t i_from = 0; i_from < stops.size(); ++i_from) {
        int span_count = 1;
        double total_dist = 0;
        for (size_t i_to = i_from + 1; i_to < stops.size(); ++i_to) { 
            auto dist = tran_catal_.FindDistance(stops[i_to - 1]->name_stop_, stops[i_to]->name_stop_);
            if (dist) {
                total_dist += *dist;
                size_t id_from = tran_catal_.FindStop(stops[i_from]->name_stop_)->id_ * 2 + 1;
                size_t id_to = tran_catal_.FindStop(stops[i_to]->name_stop_)->id_ * 2;
                graph::Edge<RouteWeight> edge{id_from, id_to, RouteWeight{(total_dist / 1000) / (bus_velocity / 60), bus->name_bus_, span_count++, true}};
                grph.AddEdge(edge);
            }
        }
    }

}

graph::DirectedWeightedGraph<JsonReader::RouteWeight> JsonReader::ProcessingRouteSettings() {
    double bus_wait_time = -1, bus_velocity = -1;
    for (const auto& [name, value] : doc_json_.GetRoot().AsMap().at("routing_settings"s).AsMap()) {
        if (name == "bus_wait_time"sv) {
            bus_wait_time = value.AsDouble();
        }
        if (name == "bus_velocity"sv) {
            bus_velocity = value.AsDouble();
        }
    }
    size_t count_stops = tran_catal_.GetStops().size();
    graph::DirectedWeightedGraph<RouteWeight> grph{count_stops * 2};
    for (int i = 0; i < count_stops; ++i) {
        size_t id_from = i * 2;
        size_t id_to = i * 2 + 1;
        grph.AddEdge(graph::Edge<RouteWeight>{id_from, id_to, RouteWeight{bus_wait_time, (tran_catal_.GetStops().begin() + i)->name_stop_, 0, false}});
    }
    for (const auto& [name, ptr_bus] : tran_catal_.GetMapBuses()) {
        if (ptr_bus->type_ == Bus::BusType::CIRCULAR) {
            ComputeEdgesCircularBus(grph, ptr_bus, bus_velocity);
        } else if (ptr_bus->type_ == Bus::BusType::DIRECT) {
            ComputeEdgesDirectBus(grph, ptr_bus, bus_velocity);
        }
    }
    return grph;
}

void JsonReader::InitialGraph(graph::DirectedWeightedGraph<RouteWeight>& graph) {
    graph = move(ProcessingRouteSettings());
}

// ----------------- реализация Json вывода статистики -----------------

json::Node::Value JsonReader::PrintInfoBus(string_view bus_view, int id) { 
    const auto info = tran_catal_.GetBusInfo(bus_view);
    json::Builder build{};
    if (info.count_stops_ == -1) {
        build.StartDict()
                .Key("error_message"s).Value("not found"s)
                .Key("request_id"s).Value(id)
            .EndDict();
    } else {
        build.StartDict()
                .Key("curvature"s).Value(info.curvature_)
                .Key("request_id"s).Value(id)
                .Key("route_length"s).Value(info.length_bus_)
                .Key("stop_count"s).Value(info.count_stops_)
                .Key("unique_stop_count"s).Value(info.count_unique_stops_)
            .EndDict();
    }
    return build.Build().GetValue();
}

json::Node::Value JsonReader::PrintInfoStop(string_view stop_view, int id) {
    const auto ptr_on_passing_buses = tran_catal_.GetPassingBuses(stop_view);
    json::Builder build{};
    if (ptr_on_passing_buses == nullptr) {
        build.StartDict()
                .Key("error_message"s).Value("not found"s)
                .Key("request_id"s).Value(id)
            .EndDict();
    } else {
        build.StartDict()
                .Key("buses"s)
                    .StartArray();
        for (auto& bus : *ptr_on_passing_buses) {
            build.Value((string)bus);
        }
                build.EndArray()
            .Key("request_id"s).Value(id)
        .EndDict();
    }
    return build.Build().GetValue();
}

json::Node::Value JsonReader::PrintInfoMap(int id) {
    visual::MapRender map_render{ProcessingRenderSettings()};
    stringstream potok;
    map_render.Render(potok, tran_catal_);
    string map, full_map;
    bool is_first = true;
    while (getline(potok, map, '\n')) {
        if (is_first) {
            is_first = false;
        } else {
            full_map += '\n';
        }
        full_map += map;
    }
    json::Builder build{};
    build.StartDict()
            .Key("map"s).Value(full_map)
            .Key("request_id"s).Value(id)
    .EndDict();
    return build.Build().GetValue();
}

json::Node::Value JsonReader::PrintInfoRoute(string_view from, string_view to, int id) {
    const static graph::Router router{graph_};

    size_t id_from = tran_catal_.FindStop(from)->id_ * 2;
    size_t id_to = tran_catal_.FindStop(to)->id_ * 2;

    auto info = router.BuildRoute(id_from, id_to);
    json::Builder build{};
    if (info) {
        build.StartDict()
                .Key("request_id"s).Value(id)
                .Key("total_time"s).Value((*info).weight.weight)
                .Key("items"s).StartArray();
        for (const auto& edge_id : (*info).edges) {
            const auto& edge = graph_.GetEdge(edge_id);
            if (edge.weight.is_bus) {
                build.StartDict()
                       .Key("type"s).Value("Bus"s)
                       .Key("bus"s).Value((string)edge.weight.name)
                       .Key("span_count"s).Value(edge.weight.span_count)
                       .Key("time"s).Value(edge.weight.weight)
                .EndDict();
            } else {
                build.StartDict()
                        .Key("type"s).Value("Wait"s)
                        .Key("stop_name"s).Value((string)edge.weight.name)
                        .Key("time"s).Value(edge.weight.weight)
                .EndDict();
            }
        }
        build.EndArray()
        .EndDict();
    } else {
        build.StartDict()
                .Key("request_id"s).Value(id)
                .Key("error_message"s).Value("not found"s)
        .EndDict();
    }
    return build.Build().GetValue();
}

void JsonReader::ProcessingRequests(ostream& output) {
    json::Builder build{};
    build.StartArray();
    for (const auto& request : doc_json_.GetRoot().AsMap().at("stat_requests"s).AsArray()) {
        json::Dict dict = request.AsMap();
        string_view type = dict.at("type"s).AsString();
        if (type == "Bus"sv) {
            build.Value(PrintInfoBus(dict.at("name"s).AsString(), dict.at("id"s).AsInt()));
        }
        if (type == "Stop"sv) {
            build.Value(PrintInfoStop(dict.at("name"s).AsString(), dict.at("id"s).AsInt()));
        }
        if (type == "Map"sv) {
            build.Value(PrintInfoMap(dict.at("id"s).AsInt()));
        }
        if (type == "Route"sv) {
            build.Value(PrintInfoRoute(dict.at("from"s).AsString(), dict.at("to"s).AsString(), dict.at("id"s).AsInt()));
        }
    }
    build.EndArray();
    json::Print(json::Document{build.Build()}, output);
}

} // namespace json_handler

} // namespace catalogue