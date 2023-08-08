#pragma once

#include <cstdint>
#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <variant>
#include <optional>

namespace svg {
    
using namespace std::literals;

struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }
    double x = 0;
    double y = 0;
};
    
/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};
    
/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

struct Rgb {
    Rgb() = default;
    
    Rgb(uint8_t red, uint8_t green, uint8_t blue)
        : red(red)
        , green(green)
        , blue(blue) {
    }
    
    uint8_t red = {};
    uint8_t green = {};
    uint8_t blue = {};
};
    
struct Rgba : public Rgb {
    Rgba() = default;
    
    Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity)
        : Rgb(red, green, blue)
        , opacity(opacity) {
    }
    
    double opacity = 1;
};
    
using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

// Объявив в заголовочном файле константу со спецификатором inline,
// мы сделаем так, что она будет одной на все единицы трансляции,
// которые подключают этот заголовок.
// В противном случае каждая единица трансляции будет использовать свою копию этой константы
inline const Color NoneColor{"none"};
    
struct OstreamColorPrinter {
    std::ostream& out;
    
    void operator()(std::monostate) const {
        out << "none"sv;
    }
    void operator()(std::string color) const {
        out << color;
    }
    void operator()(Rgb color) const {
        out << "rgb("sv << static_cast<uint16_t>(color.red) 
            << ","sv << static_cast<uint16_t>(color.green) 
            << ","sv << static_cast<uint16_t>(color.blue) << ")"sv;
    }
    void operator()(Rgba color) const {
        out << "rgba("sv << static_cast<uint16_t>(color.red)
            << ","sv << static_cast<uint16_t>(color.green)
            << ","sv << static_cast<uint16_t>(color.blue) 
            << ","sv << color.opacity << ")"sv;
    }
};

inline std::ostream& operator<< (std::ostream& out, const Color& color) {
    std::visit(OstreamColorPrinter{out}, color);
    return out;
}
    
enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};
    
enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};
   
inline std::ostream& operator<< (std::ostream& out, const StrokeLineCap& line_cap) {
    switch (line_cap) {
        case StrokeLineCap::BUTT :
            out << "butt"s;
            break;
        case StrokeLineCap::ROUND :
            out << "round"s;
            break;
        case StrokeLineCap::SQUARE :
            out << "square"s;
            break;
    }
    return out;
}

inline std::ostream& operator<< (std::ostream& out, const StrokeLineJoin& line_cap) {
    switch (line_cap) {
        case StrokeLineJoin::ARCS :
            out << "arcs"s;
            break;
        case StrokeLineJoin::BEVEL :
            out << "bevel"s;
            break;
        case StrokeLineJoin::MITER :
            out << "miter"s;
            break;
        case StrokeLineJoin::MITER_CLIP :
            out << "miter-clip"s;
            break;
        case StrokeLineJoin::ROUND :
            out << "round"s;
            break;
    }
    return out;
}
    
template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeWidth(double width) {
        stroke_width_ = width;
        return AsOwner();
    }
    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
        stroke_line_cap_ = line_cap;
        return AsOwner();
    }
    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
        stroke_line_join_ = line_join;
        return AsOwner();
    }
    
protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const {
        using namespace std::literals;

        if (fill_color_) {
            out << " fill=\""sv;
            std::visit(OstreamColorPrinter{out}, *fill_color_);
            out << "\""sv;
        }
        if (stroke_color_) {
            out << " stroke=\""sv;
            std::visit(OstreamColorPrinter{out}, *stroke_color_);
            out << "\""sv;
        }
        if (stroke_width_) {
            out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
        }
        if (stroke_line_cap_) {
            out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv;
        }
        if (stroke_line_join_) {
            out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
        }
    }

private:
    Owner& AsOwner() {
        // static_cast безопасно преобразует *this к Owner&,
        // если класс Owner — наследник PathProps
        return static_cast<Owner&>(*this);
    }

    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_line_cap_;
    std::optional<StrokeLineJoin> stroke_line_join_;
};
    
/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline> {
public:
    // Добавляет очередную вершину к ломаной линии
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;
    
    std::vector<Point> points_;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text> {
public:
    Text();
    
    // Задаёт координаты опорной точки (атрибуты x и y)
    Text& SetPosition(Point pos);

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& SetOffset(Point offset);

    // Задаёт размеры шрифта (атрибут font-size)
    Text& SetFontSize(uint32_t size);

    // Задаёт название шрифта (атрибут font-family)
    Text& SetFontFamily(std::string font_family);

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& SetFontWeight(std::string font_weight);

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& SetData(std::string data);

private:
    void RenderObject(const RenderContext& context) const override;
    
    Point position_ = {}; 
    Point offset_ = {};
    uint32_t font_size_ = 1;
    std::optional<std::string> font_family_;
    std::optional<std::string> font_weight_;
    std::string data_;
    std::unordered_map<char, std::string> shieldings_{
          {'"', "&quot;"s}
        , {'<', "&lt;"s}
        , {'>', "&gt;"s}
        , {'\'', "&apos;"s}
        , {'&', "&amp;"s}};
};

class ObjectContainer {
public:
    template <typename Obj>
    void Add(Obj obj) {
        AddPtr(std::make_unique<Obj>(std::move(obj)));
    }
    
    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
    
    virtual ~ObjectContainer() = default;
};
    
class Document : public ObjectContainer {
public:
    // Добавляет в svg-документ объект-наследник svg::Object
    void AddPtr(std::unique_ptr<Object>&& obj) override ;

    // Выводит в ostream svg-представление документа
    void Render(std::ostream& out) const;
    
private:
    std::vector<std::unique_ptr<Object>> objects_;
};

class Drawable {
public:
    virtual void Draw(ObjectContainer& container) const = 0;
    
    virtual ~Drawable() = default;
};
    
}  // namespace svg