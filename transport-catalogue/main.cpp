#include <iostream>

using namespace std;

#include "input_reader.h"
#include "transport_catalogue.h"
#include "stat_reader.h"
//#include "tests.h"

int main () {

    //catalogue::tests::TestInputReader();
    //catalogue::tests::TestTransportCatalogue();
    //catalogue::tests::TestStatReader();

    catalogue::input::InputReader input_reader(cin);
    catalogue::TransportCatalogue transport_catalogue;
    input_reader.InitialTransportCatalogue(transport_catalogue);
    catalogue::output::StatReader stat_reader(cin, cout, transport_catalogue);
    stat_reader.ProcessingRequests();

}