#include "json.h"

using namespace std;

namespace json {

namespace detail {

Node LoadNode(istream& input);

Node LoadArray(istream& input) {
    Array result;

    char c;
    for (; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    if (result.empty() && c != ']') {
        throw ParsingError("missing symbol ']'"s);
    }

    return Node(move(result));
}

using Number = std::variant<int, double>;

Node LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                //return std::stoi(parsed_num);
                return Node(std::stoi(parsed_num));
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        //return std::stod(parsed_num);
        return Node(std::stod(parsed_num));
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

Node LoadString(std::istream& input) {
    using namespace std::literals;
    
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }
    
    return Node(move(s));
}

Node LoadDict(istream& input) {
    Dict result;

    char c;
    for (; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }

        string key = LoadString(input).AsString();
        input >> c;
        result.insert({move(key), LoadNode(input)});
    }

    if (result.empty() && c != '}') {
        throw ParsingError("missing symbol '}'"s); 
    }

    return Node(move(result));
}

Node LoadNull(istream& input) {
    string n = "ull"s;
    string in;
    char c;
    bool correct = true;
    int count = 0;
    while (correct && input >> c) {
        if (c != n[count]) {
            correct = false;
            continue;
        }
        in += c;
        ++count;
    }
    if (in.size() == n.size()) {
        return Node();
    } else {
        throw ParsingError("invalid keyword"s + in);
    }
}

Node LoadBool(istream& input) {
    string t = "rue"s;
    string f = "alse"s;
    string in;
    char c;
    bool correct = true;
    int count = 0;
    while (correct && input >> c) {
        if (c != f[count] && c != t[count]) {
            correct = false;
            input.unget();
            continue;
        }
        in += c;
        ++count;
    }
    size_t size = in.size();
    if (size == t.size() && in == t) {
        return Node(true);
    } else if (size == f.size() && in == f) {
        return Node(false);
    } else {
        throw ParsingError("invalid keyword"s + in);
    }
}

Node LoadNode(istream& input) {
    char c;
    while (input >> c) {
        if (c == '[') {
            return LoadArray(input);
        } else if (c == '{') {
            return LoadDict(input);
        } else if (c == '"') {
            return LoadString(input);
        } else if (c == 'n') {
            return LoadNull(input);
        } else if (c == 't' || c == 'f') {
            return LoadBool(input);
        } else if (c == ' ') {
            continue;
        } if (c == '\\') {
            input.get();
            continue;
        } else {
            input.putback(c);
            return LoadNumber(input);
        }
    }
    throw ParsingError("here"s);
}

}  // namespace detail

bool Node::IsInt() const {
    return holds_alternative<int>(value_);
}
    
bool Node::IsDouble() const {
    return holds_alternative<int>(value_) || holds_alternative<double>(value_);
} 
    
bool Node::IsPureDouble() const {
    return holds_alternative<double>(value_);
}
    
bool Node::IsBool() const {
    return holds_alternative<bool>(value_);
}
    
bool Node::IsString() const {
    return holds_alternative<string>(value_);
}
    
bool Node::IsNull() const {
    return holds_alternative<nullptr_t>(value_);
}
    
bool Node::IsArray() const {
    return holds_alternative<Array>(value_);
}
    
bool Node::IsMap() const {
    return holds_alternative<Dict>(value_);
}
 
Node::Node(std::nullptr_t)
    : value_(nullptr) {
}
    
Node::Node(Array array)
    : value_(move(array)) {
}

Node::Node(Dict map)
    : value_(move(map)) {
}

Node::Node(int value)
    : value_(value) {
}

Node::Node(double value)
    : value_(value) {
}
    
Node::Node(bool value)
    : value_(value) {
}
  
Node::Node(string value)
    : value_(move(value)) {
}    

const Array& Node::AsArray() const {
    if (!IsArray()) {
        throw std::logic_error("logic error"s);
    }
    return get<Array>(value_);
}

const Dict& Node::AsMap() const {
    if (!IsMap()) {
        throw std::logic_error("logic error"s);
    }
    return get<Dict>(value_);
}

int Node::AsInt() const {
    if (!IsInt()) {
        throw std::logic_error("logic error"s);
    }
    return get<int>(value_);
}

double Node::AsDouble() const {
    if (!IsDouble()) {
        throw std::logic_error("logic error"s);
    }
    if (IsPureDouble()) {
        return get<double>(value_);
    } else {
        return static_cast<double>(get<int>(value_));
    }
}
    
bool Node::AsBool() const {
    if (!IsBool()) {
        throw std::logic_error("logic error"s);
    }
    return get<bool>(value_);
}
    
const string& Node::AsString() const {
    if (!IsString()) {
        throw std::logic_error("logic error"s);
    }
    return get<string>(value_);
}

Document::Document(Node root)
    : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(istream& input) {
    return Document{detail::LoadNode(input)};
}

void PrintValue(std::nullptr_t, std::ostream& out) {
    out << "null"sv << '\n';
}

void PrintValue(const string& s, std::ostream& out) {
    out << '\n' << '"';
    for (const auto& c : s) {
        switch (c) {
            case '\"':
                out << "\\\"";
                break;
            case '\\':
                out << "\\\\";
                break;
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            default:
                out << c;
        }
    }
    out << '"';
}

void PrintValue(const bool b, std::ostream& out) {
    if (b) {
        out << "true"sv << '\n';
    } else {
        out << "false"sv << '\n';
    }
}

void PrintValue(const Dict& d, std::ostream& out) {
    out << '{';
    bool is_first = true;
    for (const auto& [key, node] : d) {
        if (is_first) {
            is_first = false;
        } else {
            out << ", "sv;
        }
        PrintValue(key, out);
        out << ": "sv;
        PrintNode(node, out);
    }
    out << '}' << '\n';
}

void PrintValue(const Array& a, std::ostream& out) {
    out << '[';
    bool is_first = true;
    for (const auto& node : a) {
        if (is_first) {
            is_first = false;
        } else {
            out << ", "sv;
        }
        PrintNode(node, out);
    }
    out << ']' << '\n';
}

void PrintNode(const Node& node, std::ostream& out) {
    std::visit(
        [&out](const auto& value){ PrintValue(value, out); },
        node.GetValue());
} 

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), output);
}

}  // namespace json