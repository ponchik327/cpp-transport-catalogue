#pragma once

#include <string>
#include <vector>
#include <variant>
#include <iostream>
#include <utility>

#include "json.h"

namespace json {

class Builder {
public:
    class KeyItem;

    class DictItem;

    class ArrayItem;

    class Base {
    public:
        Base(Builder& b);
        KeyItem Key(std::string key);
        Builder& Value(Node::Value node);
        DictItem StartDict();
        ArrayItem StartArray();
        Builder&  EndDict();
        Builder&  EndArray();
        Node Build();

    private:
        Builder& builder_;
    };

    class KeyItem : public Base {
    public:
        KeyItem(Builder& b);

        DictItem Value(Node::Value node);

        KeyItem Key(std::string key) = delete;
        Builder& EndDict() = delete;
        Builder& EndArray() = delete;
        Node Build() = delete;   
    };

    class DictItem : public Base {
    public:
        DictItem(Builder& b);

        Builder& Value(Node::Value node) = delete;
        DictItem StartDict() = delete;
        ArrayItem StartArray() = delete;
        Builder& EndArray() = delete;
        Node Build() = delete;
    };

    class ArrayItem : public Base {
    public:
        ArrayItem(Builder& b);

        ArrayItem Value(Node::Value node);

        KeyItem Key(std::string key) = delete;
        Builder& EndDict() = delete;
        Node Build() = delete;
    };

public :
    KeyItem Key(std::string key);
    Builder& Value(Node::Value node);
    DictItem StartDict();
    ArrayItem StartArray();
    Builder& EndDict();
    Builder& EndArray();
    Node Build();

private:
    Node root_;
    std::vector<Node*> nodes_stack_;
    bool is_initial_ = false;

    template <typename TypeNode>
    TypeNode& Get() {
        return std::get<TypeNode>(nodes_stack_.back()->GetValue());
    }
};

} // namespace json