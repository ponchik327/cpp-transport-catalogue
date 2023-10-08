#include "serialization.h"

using namespace std;

namespace serialization {

void SerializeTransportCatalogue(ofstream& output, const catalogue::TransportCatalogue& catalogue) {
    proto_tc::TransportCatalogue object;
    
    for (const auto& stop : catalogue.GetStops()) {
        
        auto s = object.add_stop();
        s->set_name(stop.name_stop_);
        s->mutable_cord()->set_lat(stop.coordinates_.lat);
        s->mutable_cord()->set_lng(stop.coordinates_.lng);
        s->set_id(stop.id_);
    }    
    
    for (const auto& bus : catalogue.GetBuses()) {
        
        auto b = object.add_bus();
        b->set_name(bus.name_bus_);
        b->set_is_circular(bus.type_ == Bus::BusType::CIRCULAR);
        for (const auto ptr_stop : bus.ptr_stops_) {
            b->add_id_stop(ptr_stop->id_);
        }
    }
    
    for (const auto& [pair, distance] : catalogue.GetMapDistance()) {

        auto d = object.add_distance();
        d->set_id_from(catalogue.FindStop(pair.first)->id_);
        d->set_id_to(catalogue.FindStop(pair.second)->id_);
        d->set_distance(distance);
    }
    
    object.SerializeToOstream(&output);
}

catalogue::TransportCatalogue DeserializeTransportCatalogue(string file_db) {
    
    proto_tc::TransportCatalogue object;
    catalogue::TransportCatalogue catalogue;
    ifstream input(file_db, ios::binary);
    object.ParseFromIstream(&input);
    
    map<int, string> IdToStop;
    
    for (const auto& ser_stop : object.stop()) {
        if (ser_stop.name() == "") {
            continue;
        }
        geo::Coordinates coordinates{
            ser_stop.cord().lat(),
            ser_stop.cord().lng()
        };
        Stop stop{
            ser_stop.name(),
            coordinates,
            static_cast<int>(ser_stop.id())
        };
        IdToStop[stop.id_] = stop.name_stop_;
        catalogue.AddStop(move(stop));
        catalogue.AddPassingBuses(catalogue.FindStop(IdToStop.at(stop.id_))->name_stop_);
    }
    
    for (const auto& ser_bus : object.bus()) {
        vector<const Stop*> ptr_stops;
        for (const auto& id_stop : ser_bus.id_stop()) {
            ptr_stops.push_back(catalogue.FindStop(IdToStop.at(id_stop)));
        }
        Bus bus{
            ser_bus.name(),
            ptr_stops,
            (ser_bus.is_circular()) ? Bus::BusType::CIRCULAR : Bus::BusType::DIRECT
        };
        catalogue.AddBus(move(bus));
        for (const auto& id_stop : ser_bus.id_stop()) {
            string_view stop = catalogue.FindStop(IdToStop.at(id_stop))->name_stop_;
            string_view bus = catalogue.FindBus(ser_bus.name())->name_bus_;
            catalogue.AddPassingBuses(stop, bus);
        }
    }

    for (const auto& ser_distance : object.distance()) {
        string_view from = catalogue.FindStop(IdToStop.at(ser_distance.id_from()))->name_stop_;
        string_view to = catalogue.FindStop(IdToStop.at(ser_distance.id_to()))->name_stop_;
        catalogue.SetDistance(
            from, 
            to, 
            ser_distance.distance()
        );
    }

    return catalogue;
}

proto_render::Color SerializeColor(svg::Color color) {
    proto_render::Color object;
    
    if (std::holds_alternative<std::monostate>(color)) {
        object.set_is_none(true);
    } else if (std::holds_alternative<std::string>(color)) {
        object.set_name(std::get<std::string>(color));
    } else {
        bool is_rgba = std::holds_alternative<svg::Rgba>(color);

        if (is_rgba) {
            svg::Rgba rgba{std::get<svg::Rgba>(color)};
    
            object.mutable_rgba()->set_is_rgba(is_rgba);
            object.mutable_rgba()->set_red(rgba.red);
            object.mutable_rgba()->set_green(rgba.green);
            object.mutable_rgba()->set_blue(rgba.blue);
            object.mutable_rgba()->set_opacity(rgba.opacity);
        } else {
            svg::Rgb rgb{std::get<svg::Rgb>(color)};

            object.mutable_rgba()->set_is_rgba(is_rgba);
            object.mutable_rgba()->set_red(rgb.red);
            object.mutable_rgba()->set_green(rgb.green);
            object.mutable_rgba()->set_blue(rgb.blue);
        }
    }

    return object;
}

void SerializeVisualizationSettings(std::ofstream& output, const catalogue::visual::MapRender& render) {

    proto_render::MapRender object;

    object.mutable_screen()->set_width(render.GetScreen().width);
    object.mutable_screen()->set_height(render.GetScreen().height);
    object.mutable_screen()->set_padding(render.GetScreen().padding);

    object.set_stop_radius(render.GetStopRadius());
    object.set_line_width(render.GetLineWidth());

    object.mutable_bus()->mutable_offset()->set_x(render.GetLabelBus().offset.x);
    object.mutable_bus()->mutable_offset()->set_y(render.GetLabelBus().offset.y);
    object.mutable_bus()->set_font_size(render.GetLabelBus().font_size);

    object.mutable_stop()->mutable_offset()->set_x(render.GetLabelStop().offset.x);
    object.mutable_stop()->mutable_offset()->set_y(render.GetLabelStop().offset.y);
    object.mutable_stop()->set_font_size(render.GetLabelStop().font_size);

    *object.mutable_background()->mutable_color() = SerializeColor(render.GetUnderLayer().color);
    object.mutable_background()->set_width(render.GetUnderLayer().width);

    for (const auto& color : render.GetColorPalette()) {
        object.mutable_color_palette()->Add(SerializeColor(color));
    }

    object.SerializeToOstream(&output);
}

svg::Color DeserializeColor(proto_render::Color object) {
    svg::Color color;

    if (object.is_none()) {
        color = std::monostate();

    } else if (!object.name().empty()) {
        color = object.name();

    } else if (object.rgba().is_rgba()) {
        color = svg::Rgba{static_cast<uint8_t>(object.rgba().red()), 
                          static_cast<uint8_t>(object.rgba().green()), 
                          static_cast<uint8_t>(object.rgba().blue()), 
                          static_cast<double>(object.rgba().opacity())
        };
    } else {
        color = svg::Rgb{static_cast<uint8_t>(object.rgba().red()), 
                         static_cast<uint8_t>(object.rgba().green()), 
                         static_cast<uint8_t>(object.rgba().blue())
        };
    }

    return color;
}

catalogue::visual::MapRender DeserializeVisualizationSettings(string file_db) {

    proto_render::MapRender object;
    ifstream input(file_db, ios::binary);
    object.ParseFromIstream(&input);

    catalogue::visual::Properties prop;
    prop["width"] = object.screen().width();
    prop["height"] = object.screen().height();
    prop["padding"] = object.screen().padding();
    prop["stop_radius"] = object.stop_radius();
    prop["line_width"] = object.line_width();
    prop["bus_label_font_size"] = object.bus().font_size();
    prop["stop_label_font_size"] = object.stop().font_size();
    prop["underlayer_width"] = object.background().width();

    svg::Point bus_offset{object.bus().offset().x(), object.bus().offset().y()};

    svg::Point stop_offset{object.stop().offset().x(), object.stop().offset().y()};

    svg::Color underlayer_color{DeserializeColor(object.background().color())};

    std::vector<svg::Color> color_palette;
    for (const auto& color : object.color_palette()) {
        color_palette.push_back(DeserializeColor(color));
    }

    return {std::move(prop), std::move(bus_offset), std::move(stop_offset), std::move(underlayer_color), std::move(color_palette)};
}

void SerializeTransportRouter(std::ofstream& output, const TransportRouter& router) {
    proto_router::TransportRouter object;
    proto_router::Graph ser_graph;

    const auto graph = router.GetGraph();
    for (int i = 0; i < graph.GetEdgeCount(); ++i) {
        auto new_edge = ser_graph.add_edge(); 

        const auto edge = graph.GetEdge(i);
        new_edge->set_from(edge.from);
        new_edge->set_to(edge.to);

        const auto weight = edge.weight;
        new_edge->mutable_weight()->set_weight(weight.weight);
        new_edge->mutable_weight()->set_name((string)weight.name);
        new_edge->mutable_weight()->set_span_count(weight.span_count);
        new_edge->mutable_weight()->set_is_bus(weight.is_bus);
    }

    for (int i = 0; i < graph.GetVertexCount(); ++i) {
        auto incidence_list = ser_graph.add_incidence_list();
        
        for (const auto& edge_id : graph.GetIncidentEdges(i)) {
            incidence_list->add_edge_id(edge_id);
        }
    }

    *object.mutable_graph() = move(ser_graph);

    object.SerializeToOstream(&output);
}

TransportRouter DeserializeTransportRouter(std::string file_db, const catalogue::TransportCatalogue& catalogue) {
    ifstream input(file_db, ios::binary);
    proto_router::TransportRouter object;
    object.ParseFromIstream(&input);

    TransportRouter::Graph graph{static_cast<size_t>(object.graph().incidence_list_size())};

    auto size = object.graph().edge_size();

    for (const auto ser_edge : object.graph().edge()) {

        auto edge = graph::Edge<TransportRouter::RouteWeight>{ser_edge.from(), 
                                                              ser_edge.to(), 
                                                              TransportRouter::RouteWeight{ser_edge.weight().weight(),
        (ser_edge.weight().is_bus()) ? catalogue.FindBus(ser_edge.weight().name())->name_bus_ : catalogue.FindStop(ser_edge.weight().name())->name_stop_,
                                                                                           static_cast<int>(ser_edge.weight().span_count()),
                                                                                           ser_edge.weight().is_bus()                                         
                                                                                          }                
        };

        graph.AddEdge(edge);
    }

    return {catalogue, move(graph)};
}

};  // namespace serialization