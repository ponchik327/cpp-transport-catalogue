#include "json_builder.h"

using namespace std;

namespace json {

//--------------- BaseContext ----------------

Builder::Base::Base(Builder& b)
    : builder_(b) {
}

Builder::KeyItem Builder::Base::Key(string key) {
    return builder_.Key(key);
}

Builder& Builder::Base::Value(Node::Value node) {
    return builder_.Value(node);
}

Builder::DictItem Builder::Base::StartDict() {
    return builder_.StartDict();
} 

Builder::ArrayItem Builder::Base::StartArray() {
    return builder_.StartArray();
}

Builder& Builder::Base::EndDict() {
    return builder_.EndDict();
}

Builder& Builder::Base::EndArray() {
    return builder_.EndArray();
}

Node Builder::Base::Build() {
    return  builder_.Build();
}

//--------------- HelpConstructors ----------------

Builder::KeyItem::KeyItem(Builder& b)
    : Base(b) {
}

Builder::DictItem Builder::KeyItem::Value(Node::Value node) {
    return DictItem{Base::Value(node)};
}

Builder::DictItem::DictItem(Builder& b)
    : Base(b) {
}

Builder::ArrayItem::ArrayItem(Builder& b)
    : Base(b) {
}

Builder::ArrayItem Builder::ArrayItem::Value(Node::Value node) {
    return ArrayItem{Base::Value(node)};
}

//--------------- Builder ----------------

Builder::KeyItem Builder::Key(string key) {
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

Builder& Builder::InitialNewNode(const Node&& node) {
    if (nodes_stack_.empty() && is_initial_) {
        throw logic_error("was expected Build()"); 
    }
    is_initial_ = true;
    if (nodes_stack_.empty()) {
        root_ = move(node);
        nodes_stack_.push_back(&root_);
        return {*this};
    } 
    auto ptr_on_node = nodes_stack_.back();
    if (ptr_on_node->IsArray()) {
        Get<Array>().push_back(move(node));
        nodes_stack_.push_back(&Get<Array>().back());
    } else if (ptr_on_node->IsNull()) {
        *ptr_on_node = move(node);
    }
    return {*this};
}

Builder::DictItem Builder::StartDict() {
    Node d{Dict{}};
    return {InitialNewNode(move(d))};
    throw logic_error("Uncorrect use StartDict()");
}

Builder::ArrayItem Builder::StartArray() {
    Node ar{Array{}};
    return {InitialNewNode(move(ar))};
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