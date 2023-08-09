#include "json_builder.h"

using namespace std;

namespace json {

namespace context {

//--------------- BaseContext ----------------

Base::Base(Builder& b)
    : builder_(b) {
}

KeyItem Base::Key(string key) {
    return builder_.Key(key);
}

Builder& Base::Value(Node::Value node) {
    return builder_.Value(node);
}

DictItem Base::StartDict() {
    return builder_.StartDict();
} 

ArrayItem Base::StartArray() {
    return builder_.StartArray();
}

Builder& Base::EndDict() {
    return builder_.EndDict();
}

Builder& Base::EndArray() {
    return builder_.EndArray();
}

Node Base::Build() {
    return  builder_.Build();
}

//--------------- HelpConstructors ----------------

KeyItem::KeyItem(Builder& b)
    : Base(b) {
}

AfterKey KeyItem::Value(Node::Value node) {
    return AfterKey{Base::Value(node)};
}

DictItem::DictItem(Builder& b)
    : Base(b) {
}

ArrayItem::ArrayItem(Builder& b)
    : Base(b) {
}

AfterStartArray ArrayItem::Value(Node::Value node) {
    return AfterStartArray{Base::Value(node)};
}

AfterKey::AfterKey(Builder& b)
    : Base(b) {
}

AfterStartArray::AfterStartArray(Builder& b)
    : Base(b) {
}

AfterStartArray AfterStartArray::Value(Node::Value node) {
    return AfterStartArray{Base::Value(node)};
}

} // namespace context

//--------------- Builder ----------------

context::KeyItem Builder::Key(string key) {
    if (nodes_stack_.empty()) {
        throw logic_error("was expected Build()"); 
    }
    if (!nodes_stack_.back()->IsMap()) {
        throw logic_error("use Key() outside map");
    }
    if (nodes_stack_.back()->IsNull()) {
        throw logic_error("was expected Value()");
    }
    nodes_stack_.push_back(&Get<Dict>()[move(key)]);
    return {*this};
}

Builder& Builder::Value(Node::Value val_node) {
    if (nodes_stack_.empty() && is_initial_) {
        throw logic_error("was expected Build()"); 
    }
    is_initial_ = true;
    if (nodes_stack_.empty()) {
        Node node{};
        node.GetValue() = move(val_node);
        root_ = move(node);
        return *this;
    } 
    auto ptr_on_node = nodes_stack_.back();
    if (ptr_on_node->IsNull()) {
        ptr_on_node->GetValue() = move(val_node);
        nodes_stack_.pop_back();
        return *this;
    } else if (ptr_on_node->IsArray()) {
        Node node{};
        node.GetValue() = move(val_node); 
        Get<Array>().push_back(move(node));
        return *this;
    }
    throw logic_error("Uncorrect use Value()");
}

context::DictItem Builder::StartDict() {
    if (nodes_stack_.empty() && is_initial_) {
        throw logic_error("was expected Build()"); 
    }
    is_initial_ = true;
    Node d{Dict{}};
    if (nodes_stack_.empty()) {
        root_ = move(d);
        nodes_stack_.push_back(&root_);
        return {*this};
    } 
    auto ptr_on_node = nodes_stack_.back();
    if (ptr_on_node->IsArray()) {
        Get<Array>().push_back(move(d));
        nodes_stack_.push_back(&Get<Array>().back());
        return {*this};
    } else if (ptr_on_node->IsNull()) {
        *ptr_on_node = move(d);
        return {*this};
    }
    throw logic_error("Uncorrect use StartDict()");
}

context::ArrayItem Builder::StartArray() {
    if (nodes_stack_.empty() && is_initial_) {
        throw logic_error("was expected Build()"); 
    }
    is_initial_ = true;
    Node ar{Array{}};
    if (nodes_stack_.empty()) {
        root_ = move(ar);
        nodes_stack_.push_back(&root_);
        return {*this};
    } 
    auto ptr_on_node = nodes_stack_.back();
    if (ptr_on_node->IsArray()) {
        Get<Array>().push_back(move(ar));
        nodes_stack_.push_back(&Get<Array>().back());
        return {*this};
    } else if (ptr_on_node->IsNull()) {
        *ptr_on_node = move(ar);
        return {*this};
    }
    throw logic_error("Uncorrect use StartArray()");
}

Builder& Builder::EndDict() {
    if (nodes_stack_.empty()) {
        throw logic_error("was expected Build()"); 
    }
    if (!nodes_stack_.back()->IsMap()) {
        throw logic_error("Uncorrect use EndDict()");
    }
    nodes_stack_.pop_back();
    return *this;
}

Builder& Builder::EndArray() {
    if (nodes_stack_.empty()) {
        throw logic_error("was expected Build()"); 
    }
    if (!nodes_stack_.back()->IsArray()) {
        throw logic_error("Uncorrect use EndArray()");
    }
    nodes_stack_.pop_back();
    return *this;
}

Node Builder::Build() {
    if (!nodes_stack_.empty()) {
        throw logic_error("not end array or map");
    }
    if (!is_initial_) {
        throw logic_error("empty builder");
    }
    return root_;
}

} // namespace json 