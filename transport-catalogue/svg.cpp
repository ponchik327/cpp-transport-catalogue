#include "svg.h"

namespace svg {

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\" "sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}

// ---------- Polyline ------------------ 
   
Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}
    
void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    bool is_first = true;
    for (auto& point : points_) {
        if (is_first) {
            out << point.x << ","sv << point.y;
            is_first = false;
        } else {
            out << " "sv << point.x << ","sv << point.y;
        }
    }
    out << "\""sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}
    
// ---------- Text ------------------

Text::Text() {
}
    
Text& Text::SetPosition(Point pos) {
    position_ = pos;
    return *this;
}
    
Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}
    
Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}
    
Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = move(font_family);
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = move(font_weight);
    return *this;
}

Text& Text::SetData(std::string data) {
    std::string new_data;
    for (auto c : data) {
        if (shieldings_.count(c)) {
            new_data += shieldings_.at(c);
        } else {
            new_data += std::move(c);
        }
    }
    data_ = std::move(new_data);
    return *this;
}
    
void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text"sv;
    RenderAttrs(context.out);
    out << " x=\""sv << position_.x << "\""sv;
    out << " y=\""sv << position_.y << "\""sv;
    out << " dx=\""sv << offset_.x << "\""sv;
    out << " dy=\""sv << offset_.y << "\""sv;
    out << " font-size=\""sv << font_size_ << "\""sv;
    if (font_family_) {
        out << " font-family=\""sv << *font_family_ << "\""sv;
    }
    if (font_weight_) {
        out << " font-weight=\""sv << *font_weight_ << "\""sv;
    }
    out << ">"sv << data_ << "</text>"sv;
}
 
// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.push_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
    RenderContext ctx(out, 2, 2);
    for (auto& ptr_obj : objects_) {
        ptr_obj->Render(ctx);
    }
    out << "</svg>"sv << std::endl;
}
    
}  // namespace svg