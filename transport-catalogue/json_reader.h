#pragma once
#include <iostream>
#include <sstream>
#include <string_view>
#include <iterator>

#include "transport_catalogue.h"
#include "transport_router.h"
#include "json_builder.h"
#include "json.h"
#include "map_renderer.h"
#include "router.h"
#include "graph.h"

namespace catalogue {

namespace json_handler {

enum class RequestType {
    BUS,
    STOP,
};

// ------------- Json ридер и вывод статистики -------------

class JsonReader {
public:
    using Request = json::Dict;
    using RequestQueque = std::deque<std::pair<Request, RequestType>>;

    JsonReader(std::istream& input, TransportCatalogue& t_c);

    void MakeRequestQueue();
    RequestQueque& GetRequestQueue();
    void ProcessingRequests(std::ostream& output);

private:
    std::istream& input_;
    RequestQueque request_queue_add_;
    TransportCatalogue& tran_catal_;
    const json::Document doc_json_;

    void InitialTransportCatalogue(TransportCatalogue& t_c);
    Stop ReadStop(Request& request, TransportCatalogue& t_c);
    Bus ReadBus(Request& request, TransportCatalogue& t_c);
    json::Node::Value PrintInfoBus(std::string_view bus_view, int id);
    json::Node::Value PrintInfoStop(std::string_view stop_view, int id);

    visual::MapRender ProcessingRenderSettings();
    uint8_t NodeToUint8(const json::Node& color, int index);
    svg::Color InitialColor(const json::Node& color);
    json::Node::Value PrintInfoMap(int id);
    
    TransportRouter ProcessingRouterSettings();
    json::Node::Value PrintInfoRoute(std::string_view from, std::string_view to, int id);
}; 

} // namespace json_handler

} // namespace catalogue