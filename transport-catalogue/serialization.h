#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <string>
#include <transport_catalogue.pb.h>
#include <map_renderer.pb.h>
#include <transport_router.pb.h>

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

namespace serialization {

//using Path = std::filesystem::path;
    
void SerializeTransportCatalogue(std::ofstream& output, const catalogue::TransportCatalogue& catalogue);
catalogue::TransportCatalogue DeserializeTransportCatalogue(std::string file_db);

void SerializeVisualizationSettings(std::ofstream& output, const catalogue::visual::MapRender& render);
catalogue::visual::MapRender DeserializeVisualizationSettings(std::string file_db);

void SerializeTransportRouter(std::ofstream& output, const TransportRouter& router);
TransportRouter DeserializeTransportRouter(std::string file_db, const catalogue::TransportCatalogue& catalogue);

};  // namespace serialization