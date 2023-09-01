#pragma once
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <cstdlib>
#include <optional>

#include "svg.h"
#include "geo.h"
#include "transport_catalogue.h"

namespace catalogue {

namespace visual {

namespace detail {

class SphereProjector;

} // namespace detail

using Properties = std::unordered_map<std::string, double>;

class MapRender {
public:
    MapRender(Properties prop, svg::Point bus_offset, svg::Point stop_offset, svg::Color underlayer_color, std::vector<svg::Color> color_palette);
    void Render(std::ostream& out, TransportCatalogue& t_c);

private:
    Properties properties_;
    svg::Point bus_label_offset_;
    svg::Point stop_label_offset_;
    svg::Color underlayer_color_;
    std::vector<svg::Color> color_palette_;

    void RenderBusLine(svg::Document& doc, std::vector<std::vector<geo::Coordinates>>& bus_coordinates, detail::SphereProjector& sph_proj);

    void RenderNameBuses(svg::Document& doc, std::set<std::string_view>& name_bases, TransportCatalogue& t_c, detail::SphereProjector& sph_proj);
    void RenderText(svg::Document& doc, const Stop* ptr_stop, std::string_view bus_name, size_t index, detail::SphereProjector& sph_proj);

    void RenderStopSymbol(svg::Document& doc, std::set<std::string_view>& name_stops, TransportCatalogue& t_c, detail::SphereProjector& sph_proj);
    
    void RenderNameStops(svg::Document& doc, std::set<std::string_view>& name_stops, TransportCatalogue& t_c, detail::SphereProjector& sph_proj);
};

namespace detail {

inline const double EPSILON = 1e-6;
bool IsZero(double value);

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width, double max_height, double padding);

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const ;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

} // namespace detail

} // namespace visual

} // namespace catalogue

template <typename PointInputIt>
catalogue::visual::detail::SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width, double max_height, double padding)
    : padding_(padding) //
{
    // Если точки поверхности сферы не заданы, вычислять нечего
    if (points_begin == points_end) {
        return;
    }

    // Находим точки с минимальной и максимальной долготой
    const auto [left_it, right_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
    min_lon_ = left_it->lng;
    const double max_lon = right_it->lng;

    // Находим точки с минимальной и максимальной широтой
    const auto [bottom_it, top_it] = std::minmax_element(
        points_begin, points_end,
        [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
    const double min_lat = bottom_it->lat;
    max_lat_ = top_it->lat;

    // Вычисляем коэффициент масштабирования вдоль координаты x
    std::optional<double> width_zoom;
    if (!IsZero(max_lon - min_lon_)) {
        width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
    }

    // Вычисляем коэффициент масштабирования вдоль координаты y
    std::optional<double> height_zoom;
    if (!IsZero(max_lat_ - min_lat)) {
        height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
    }

    if (width_zoom && height_zoom) {
        // Коэффициенты масштабирования по ширине и высоте ненулевые,
        // берём минимальный из них
        zoom_coeff_ = std::min(*width_zoom, *height_zoom);
    } else if (width_zoom) {
        // Коэффициент масштабирования по ширине ненулевой, используем его
        zoom_coeff_ = *width_zoom;
    } else if (height_zoom) {
        // Коэффициент масштабирования по высоте ненулевой, используем его
        zoom_coeff_ = *height_zoom;
    }
}
