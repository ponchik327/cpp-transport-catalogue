#include "input_reader.h"

using namespace std;

namespace catalogue {

namespace input {

InputReader::InputReader(istream& input)
    : input_(input) 
{}

void InputReader::ProcessingDistance(int coma, string_view name, string_view request, TransportCatalogue& t_c) {
    while (!request.empty()) {
        request.remove_prefix(coma + 2);
        int pos_m = request.find('m');
        int distance = StringToNum<int>((string)request.substr(0, pos_m));
        request.remove_prefix(pos_m + 5);
        coma = request.find(',');
        if (coma == request.npos) { 
            t_c.SetDistance(name, request, distance);
            request.remove_prefix(request.size());
            break;   
        }
        t_c.SetDistance(name, request.substr(0, coma), distance);
    }
}

Stop InputReader::ReadStop(string_view request, TransportCatalogue& t_c) {
    int space = request.find(' ');
    int colon = request.find(':', space);
    string_view name = request.substr(space + 1, colon - space - 1);

    int coma = request.find(',', colon);
    string latitude_str = (string)request.substr(colon + 2, coma - colon - 2);
    int prev_coma = coma;
    coma = request.find(',', prev_coma + 1);
    string longitude_str = (string)request.substr(prev_coma + 2, coma - prev_coma - 2); 

    if (coma != request.npos) {
        ProcessingDistance(coma, name, request, t_c);
    }

    t_c.AddPassingBuses(name);

    return Stop{(string)name, StringToNum<double>(move(latitude_str)), StringToNum<double>(move(longitude_str))};
}

pair<vector<string_view>, Bus::BusType> InputReader::BusStopsProcessing(string_view request) {
    size_t end = request.npos;
    bool is_find_dash = (request.find('-') != end);
    char delim = (is_find_dash) ? '-' : '>';
    Bus::BusType type = (is_find_dash) ? Bus::BusType::DIRECT : Bus::BusType::CIRCULAR;

    vector<string_view> stops_view, buffer;
    while (!request.empty()) {
        size_t pos = request.find(delim);
        if (pos == end) {
            stops_view.push_back(request);
            request.remove_prefix(request.size());
            break;
        }
        string_view stop = request.substr(0, pos - 1);
        stops_view.push_back(stop);
        if (delim == '-') {
            buffer.push_back(stop);
        }
        request.remove_prefix(pos + 2);
    }

    if (delim == '-') {
        for (auto it = buffer.rbegin(); it < buffer.rend(); ++it) {
            stops_view.push_back(move(*it));
        }
    }

    return {move(stops_view), type};
}

Bus InputReader::ReadBus(string_view request, TransportCatalogue& t_c) {
    int space = request.find(' ');
    int colon = request.find(':', space);
    string_view name = request.substr(space + 1, colon - space - 1);

    request.remove_prefix(colon + 2);
    vector<Stop*> stops;
    auto [stops_view, type] = BusStopsProcessing(request);
    for (auto& stop_view : stops_view) {
        t_c.AddPassingBuses(stop_view, name);
        Stop* stop = t_c.FindStop(stop_view);
        stops.push_back(stop);
    }

    return Bus{(string)name, move(stops), type};
}

void InputReader::MakeRequestQueue() {
    int n;
    input_ >> n;
    string num;
    getline(input_, num);
    for (int i = 0; i < n; ++i) {
        string request;
        getline(input_, request);
        string_view defenition_request = request;
        defenition_request = defenition_request.substr(0, defenition_request.find(' '));
        if (defenition_request == "Stop") {
            request_queue_.push_front({move(request), RequestType::STOP});
        }
        if (defenition_request == "Bus") {
            request_queue_.push_back({move(request), RequestType::BUS});
        }
    }
}

deque<pair<string, InputReader::RequestType>> InputReader::GetRequestQueue() {
    return request_queue_;
}

void InputReader::InitialTransportCatalogue(TransportCatalogue& t_c) {
    MakeRequestQueue();
    for (auto& [request, type] : request_queue_) {
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

} // namespace input 

} // namespace catalogue 