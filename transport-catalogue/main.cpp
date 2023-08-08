#include <iostream>
#include <fstream>

using namespace std;

#include "input_reader.h"
#include "transport_catalogue.h"
#include "stat_reader.h"

int main () {
    ifstream in("input.txt");
    ofstream out("output.txt");

    catalogue::input::InputReader input_reader(in);
    catalogue::TransportCatalogue transport_catalogue;
    input_reader.InitialTransportCatalogue(transport_catalogue);
    catalogue::output::StatReader stat_reader(in, out, transport_catalogue);
    stat_reader.ProcessingRequests();

}