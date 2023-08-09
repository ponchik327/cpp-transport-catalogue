#pragma once

#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include <variant>
#include <type_traits>
#include <stdexcept>

namespace json {

class Node;
// Сохраните объявления Dict и Array без изменения
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node {
public:
   /* Реализуйте Node, используя std::variant */
    using Value = std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict>;

    Node() = default;
    Node(std::nullptr_t);
    Node(Array array);
    Node(Dict map);
    Node(int value);
    Node(double value);
    Node(bool value);
    Node(std::string value);

    bool IsInt() const;
    bool IsDouble() const; 
    bool IsPureDouble() const; 
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;
    
    int AsInt() const;
    double AsDouble() const; 
    bool AsBool() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    bool operator== (const Node& rhs) const {
        return value_ == rhs.value_;
    }
    
    bool operator!= (const Node& rhs) const {
        return value_ != rhs.value_;
    }
    
    const Value& GetValue() const { return value_; }

    Value& GetValue() { return value_; }

private:
    Value value_;
};

class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

    bool operator== (const Document& rhs) {
        return root_ == rhs.root_;
    }

    bool operator!= (const Document& rhs) {
        return root_ != rhs.root_;
    }
    
private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

void PrintNode(const Node& node, std::ostream& out);

void PrintValue(std::nullptr_t, std::ostream& out);

void PrintValue(const std::string& s, std::ostream& out);

void PrintValue(const bool b, std::ostream& out);

void PrintValue(const Dict& d, std::ostream& out);

void PrintValue(const Array& a, std::ostream& out);

template <typename V>
void PrintValue(const V& value, std::ostream& out) {
    out << value;
}

}  // namespace json