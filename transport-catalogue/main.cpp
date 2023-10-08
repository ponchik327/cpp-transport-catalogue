#include <fstream>
#include <iostream>
#include <string_view>

#include "json_reader.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "serialization.h"

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    std::ifstream in_file_make("in_out/input_make_base.json");
    std::ifstream in_file_process("in_out/input_process_requests.json");
    std::ofstream out_file("in_out/output.json");

    if (mode == "make_base"sv) {
        catalogue::json_handler::JsonReader json_reader{in_file_make, true};

    } else if (mode == "process_requests"sv) {
        catalogue::json_handler::JsonReader json_reader{in_file_process};
        json_reader.ProcessingRequests(out_file);

    } else {
        PrintUsage();
        return 1;
    }
}