#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

#include "json_reader.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

int main () {
    ifstream in("input.json");
    ofstream out("output.json");

    catalogue::TransportCatalogue transport_catalogue;
    catalogue::json_handler::JsonReader json_reader(in, transport_catalogue);
    json_reader.ProcessingRequests(out);
}