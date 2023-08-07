#pragma once
#include <iostream>
#include <iomanip>
#include <string_view>

#include "transport_catalogue.h"

using namespace std::literals;

namespace catalogue {

namespace output {

class StatReader {

public:
    StatReader(std::istream& input, std::ostream& output, TransportCatalogue& t_c);
    void ProcessingRequests();

private:
    std::istream& input_;
    std::ostream& output_;
    TransportCatalogue& tran_catal_;

    void PrintInfoBus(std::string_view bus_view);
    void PrintInfoStop(std::string_view stop_view);
    
};

} // namespace output

} // namespace catalogue