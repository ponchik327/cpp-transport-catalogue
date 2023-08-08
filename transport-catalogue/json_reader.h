#pragma once
#include <iostream>
#include <sstream>
#include <string_view>

#include "transport_catalogue.h"
#include "json.h"
#include "map_renderer.h"

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
    void InitialTransportCatalogue(TransportCatalogue& t_c);
    visual::MapRender ProcessingRenderSettings();
    void ProcessingRequests(std::ostream& output);

private:
    std::istream& input_;
    RequestQueque request_queue_add_;
    TransportCatalogue& tran_catal_;
    const json::Document doc_json_;

    Stop ReadStop(Request& request, TransportCatalogue& t_c);
    Bus ReadBus(Request& request, TransportCatalogue& t_c);

    void PrintInfoBus(std::ostream& output, std::string_view bus_view, int id);
    void PrintInfoStop(std::ostream& output, std::string_view stop_view, int id);
    void PrintInfoMap(std::ostream& output, int id);

    uint8_t NodeToUint8(const json::Node& color, int index);
    svg::Color InitialColor(const json::Node& color);

}; 

} // namespace json_handler

} // namespace catalogue