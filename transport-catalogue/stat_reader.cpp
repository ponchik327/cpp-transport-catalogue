#include "stat_reader.h"

using namespace std;

namespace catalogue {

namespace output {

StatReader::StatReader(istream& input, ostream& output, TransportCatalogue& t_c)
    : input_(input)
    , output_(output)
    , tran_catal_(t_c) 
{}

void StatReader::PrintInfoBus(string_view bus_view) {
    const auto info = tran_catal_.GetBusInfo(bus_view);
    if (info.count_stops_ == -1) {
        output_ << "Bus "s << bus_view << ": not found"s << endl;
    } else {
        output_ << "Bus "s << bus_view << ": "s << info.count_stops_ 
            << " stops on route, "s << info.count_unique_stops_ << " unique stops, "s 
            << setprecision(6) << info.length_bus_ << " route length, "s 
            << info.curvature_ << " curvature"s << endl;
    }
}

void StatReader::PrintInfoStop(string_view stop_view) {
    const auto ptr_on_passing_buses = tran_catal_.GetPassingBuses(stop_view);
    if (ptr_on_passing_buses == nullptr) {
        output_ << "Stop "s << stop_view << ": not found"s << endl;
    } else if (ptr_on_passing_buses->empty()) {
        output_ << "Stop "s << stop_view << ": no buses"s << endl;
    } else {
        output_ << "Stop "s << stop_view << ": buses"s;
        for (auto& bus : *ptr_on_passing_buses) {
            output_ << " "s << bus;
        }
        output_ << endl;
    }
}

void StatReader::ProcessingRequests() {
    int n;
    input_ >> n;
    string num;
    getline(input_, num);
    for (int i = 0; i < n; ++i) {
        string request;
        getline(input_, request);
        string_view defenition_request = request;
        defenition_request = defenition_request.substr(0, defenition_request.find(' '));
        string_view pure_request = request;
        pure_request.remove_prefix(pure_request.find(' ') + 1);
        if (defenition_request == "Stop"sv) {
            PrintInfoStop(pure_request);
        }
        if (defenition_request == "Bus"sv) {
            PrintInfoBus(pure_request);
        }
    }
}

} // namespace output

} // namespace catalogue