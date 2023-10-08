#include "map_renderer.h"

using namespace std;

namespace catalogue {

namespace visual {

MapRender::MapRender(Properties prop, svg::Point bus_offset, svg::Point stop_offset, svg::Color underlayer_color, std::vector<svg::Color> color_palette) 
    : properties_(move(prop))
    , bus_label_offset_(bus_offset)
    , stop_label_offset_(stop_offset)
    , underlayer_color_(underlayer_color)
    , color_palette_(move(color_palette)) {
}

Label MapRender::GetLabelStop() const {
    return {properties_.at("stop_label_font_size"), stop_label_offset_};
}

Label MapRender::GetLabelBus() const {
    return {properties_.at("bus_label_font_size"), bus_label_offset_};
}

Screen MapRender::GetScreen() const {
    return {properties_.at("width"), properties_.at("height"), properties_.at("padding")};
}

UnderLayer MapRender::GetUnderLayer() const {
    return {underlayer_color_, properties_.at("underlayer_width")};
}

double MapRender::GetStopRadius() const {
    return properties_.at("stop_radius");
}

double MapRender::GetLineWidth() const {
    return properties_.at("line_width");
}

const std::vector<svg::Color>& MapRender::GetColorPalette() const {
    return color_palette_;
}


void MapRender::RenderBusLine(svg::Document& doc, vector<vector<geo::Coordinates>>& bus_coordinates, detail::SphereProjector& sph_proj) {
    size_t index = 0;
    for (const auto& coordinates : bus_coordinates) {
        svg::Polyline bus;
        bus.SetStrokeColor(color_palette_[index % color_palette_.size()])
            .SetFillColor("none"s)
            .SetStrokeWidth(properties_.at("line_width"s))
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        if (!coordinates.empty()) {
            ++index;
        }
        for (const auto& coordinate : coordinates) {
            bus.AddPoint(sph_proj(coordinate));
        }
        doc.Add(move(bus));
    }  
}

void MapRender::RenderText(svg::Document& doc, const Stop* ptr_stop, string_view bus_name, size_t index, detail::SphereProjector& sph_proj) {
    svg::Text text_preparation;
    text_preparation.SetPosition(sph_proj({ptr_stop->coordinates_.lat, ptr_stop->coordinates_.lng}))
                    .SetOffset(bus_label_offset_)
                    .SetFontSize(properties_.at("bus_label_font_size"s))
                    .SetFontFamily("Verdana"s)
                    .SetFontWeight("bold"s)
                    .SetData((string)bus_name);
        
    svg::Text name_background = text_preparation;
    name_background.SetFillColor(underlayer_color_)
                    .SetStrokeColor(underlayer_color_)
                    .SetStrokeWidth(properties_.at("underlayer_width"s))
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    doc.Add(move(name_background));
        
    svg::Text name_bus = move(text_preparation);
    name_bus.SetFillColor(color_palette_[index % color_palette_.size()]);
    doc.Add(move(name_bus));
}

void MapRender::RenderNameBuses(svg::Document& doc, set<string_view>& name_bases, TransportCatalogue& t_c, detail::SphereProjector& sph_proj) {
    size_t index = 0;
    for (const auto& bus_name : name_bases) {
        auto ptr_on_stops = t_c.FindBus(bus_name)->ptr_stops_;
        const Stop* ptr_first_stop;
        const Stop* ptr_last_stop;
        if (!ptr_on_stops.empty()) {
            ptr_first_stop = ptr_on_stops.front();
            ptr_last_stop = ptr_on_stops[ptr_on_stops.size() / 2];
        } else {
            continue;
        } 

        RenderText(doc, ptr_first_stop, bus_name, index, sph_proj);
        
        if (t_c.FindBus(bus_name)->type_ == Bus::BusType::DIRECT && ptr_first_stop != ptr_last_stop) {
            RenderText(doc, ptr_last_stop, bus_name, index, sph_proj);
        }

        if (!ptr_on_stops.empty()) {
            ++index;
        }
    }
}

void MapRender::RenderStopSymbol(svg::Document& doc, set<string_view>& name_stops, TransportCatalogue& t_c, detail::SphereProjector& sph_proj) {
    for (const auto& stop_name : name_stops) {
        auto ptr_stop = t_c.FindStop(stop_name);
        svg::Circle stop_symbol;
        stop_symbol.SetCenter(sph_proj({ptr_stop->coordinates_.lat, ptr_stop->coordinates_.lng}))
                .SetRadius(properties_.at("stop_radius"s))
                .SetFillColor("white"s);
        doc.Add(move(stop_symbol));
    }
}

void MapRender::RenderNameStops(svg::Document& doc, set<string_view>& name_stops, TransportCatalogue& t_c, detail::SphereProjector& sph_proj) {
    for (const auto& stop_name : name_stops) {
        auto ptr_stop = t_c.FindStop(stop_name);
        svg::Text text_preparation;
        text_preparation.SetPosition(sph_proj({ptr_stop->coordinates_.lat, ptr_stop->coordinates_.lng}))
                        .SetOffset(stop_label_offset_)
                        .SetFontSize(properties_.at("stop_label_font_size"s))
                        .SetFontFamily("Verdana"s)
                        .SetData((string)stop_name);
        
        svg::Text name_background = text_preparation;
        name_background.SetFillColor(underlayer_color_)
                        .SetStrokeColor(underlayer_color_)
                        .SetStrokeWidth(properties_.at("underlayer_width"s))
                        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        doc.Add(move(name_background));
        
        svg::Text name_stop = move(text_preparation);
        name_stop.SetFillColor("black"s);
        doc.Add(move(name_stop));
    }
}

void MapRender::Render(std::ostream& out, TransportCatalogue& t_c) {
    svg::Document doc;
    auto buses = t_c.GetMapBuses();
    set<string_view> name_bases;
    for (const auto& [bus_name , ptr_on_bus] : buses) {
        name_bases.insert(bus_name);
    }

    vector<geo::Coordinates> all_coordinates;
    vector<vector<geo::Coordinates>> bus_coordinates;
    for (const auto& bus_name : name_bases) {
        vector<geo::Coordinates> coordinates;
        for (const auto& ptr_on_stop : t_c.FindBus(bus_name)->ptr_stops_) {
            geo::Coordinates coord{ptr_on_stop->coordinates_.lat, ptr_on_stop->coordinates_.lng};
            all_coordinates.push_back(coord);
            coordinates.push_back(coord);
        }
        bus_coordinates.push_back(coordinates);
    }
    detail::SphereProjector sph_proj{all_coordinates.begin(), all_coordinates.end(), properties_.at("width"s), properties_.at("height"s), properties_.at("padding"s)};

    RenderBusLine(doc, bus_coordinates, sph_proj);
    RenderNameBuses(doc, name_bases, t_c, sph_proj);

    set<string_view> name_stops;
    for (const auto& [bus_name , ptr_on_bus] : buses) {
        for (const auto ptr_stop : t_c.FindBus(bus_name)->ptr_stops_) {
            name_stops.insert(ptr_stop->name_stop_);
        }
    }

    RenderStopSymbol(doc, name_stops, t_c, sph_proj);
    RenderNameStops(doc, name_stops, t_c, sph_proj);
    doc.Render(out);
}

namespace detail {

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    return {
        (coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

} // namespace detail

} // namespace visual

} // namespace catalogue