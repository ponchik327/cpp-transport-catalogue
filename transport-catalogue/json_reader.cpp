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
    return Stop{(string)name_from, lat, lng};
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

// ----------------- реализация Json вывода статистики -----------------

void JsonReader::PrintInfoBus(ostream& output, string_view bus_view, int id) { 
    const auto info = tran_catal_.GetBusInfo(bus_view);
    if (info.count_stops_ == -1) {
        output << '\n' << "\"error_message\": "sv << "\"not found\","sv;
        output << '\n' << "\"request_id\": "sv << id;
    } else {
        output << '\n' << "\"curvature\": "sv << info.curvature_ << ","sv;
        output << '\n' << "\"request_id\": "sv << id << ","sv;
        output << '\n' << "\"route_length\": "sv << info.length_bus_ << ","sv;
        output << '\n' << "\"stop_count\": "sv << info.count_stops_ << ","sv;
        output << '\n' << "\"unique_stop_count\": "sv << info.count_unique_stops_;
    }
}

void JsonReader::PrintInfoStop(ostream& output, string_view stop_view, int id) {
    const auto ptr_on_passing_buses = tran_catal_.GetPassingBuses(stop_view);
    if (ptr_on_passing_buses == nullptr) {
        output << '\n' << "\"error_message\": "sv << "\"not found\","sv;
        output << '\n' << "\"request_id\": "sv << id;
    } else {
        output << "\"buses\": ["sv << '\n';
        bool is_first = true;
        for (auto& bus : *ptr_on_passing_buses) {
            if (is_first) {
                is_first = false;
            } else {
                output << ',' << '\n';    
            }
            output << '"' << bus << '"';
        }
        output << '\n' << "],"sv << '\n';
        output << '\n' << "\"request_id\": "sv << id;
    }
}

void JsonReader::PrintInfoMap(ostream& output, int id) {
    visual::MapRender map_render{ProcessingRenderSettings()};
    output << "\"map\": "sv;
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
    json::PrintValue(full_map, output);
    output << ",\n"sv;
    output << "\"request_id\": "sv << id << "\n"sv;
}

void JsonReader::ProcessingRequests(ostream& output) {
    output << '[' << '\n';
    bool is_first = true;
    for (const auto& request : doc_json_.GetRoot().AsMap().at("stat_requests"s).AsArray()) {
        if (is_first) {
            is_first = false;
        } else {
            output << ',' << '\n';
        }
        output << " {"sv;
        json::Dict dict = request.AsMap();
        string_view type = dict.at("type"s).AsString();
        if (type == "Bus"sv) {
            PrintInfoBus(output, dict.at("name"s).AsString(), dict.at("id"s).AsInt());
        }
        if (type == "Stop"sv) {
            PrintInfoStop(output, dict.at("name"s).AsString(), dict.at("id"s).AsInt());
        }
        if (type == "Map"sv) {
            PrintInfoMap(output, dict.at("id"s).AsInt());
        }
        output << " }"sv;
    }
    output << ']';
}

} // namespace json_handler

} // namespace catalogue