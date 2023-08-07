#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <utility>

#include "transport_catalogue.h"

using namespace std::literals;

namespace catalogue {

namespace input {

class InputReader {

enum class RequestType {
    BUS,
    STOP,
};

public:
    InputReader(std::istream& input);
    void MakeRequestQueue();
    std::deque<std::pair<std::string, RequestType>> GetRequestQueue();
    void InitialTransportCatalogue(TransportCatalogue& t_c);

private:
    std::istream& input_;
    std::deque<std::pair<std::string, RequestType>> request_queue_;

    template <typename T>
    T StringToNum(std::string value);
    Stop ReadStop(std::string_view request, TransportCatalogue& t_c);
    void ProcessingDistance(int coma, std::string_view name, std::string_view request, TransportCatalogue& t_c);
    std::pair<std::vector<std::string_view>, Bus::BusType> BusStopsProcessing(std::string_view request);
    Bus ReadBus(std::string_view request, TransportCatalogue& t_c);

};

template <typename T>
T InputReader::StringToNum(std::string value) {
    std::istringstream potok;
    potok.str(move(value));
    T num;
    potok >> num;
    return num;
}

} // namespace input 

} // namespace catalogue 