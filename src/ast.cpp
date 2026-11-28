#include "ast.hpp"
#include <iostream>


// makes a new node with nothing on its sides and gives back where it is
Node* make_node(NodeKind kind, string text, int line) {
    //a new empty node in memory, node holds its address
    Node* node = new Node;
    //fill in the kind
    node->kind = kind;
    //fill in the text
    node->text = text;
    //fill in the line
    node->line = line;
    //give back the address
    return node;
}

// makes an operator node with two sides and gives back where it is
Node* make_binary_node(string op, Node* left_side, Node* right_side, int line) {
    //a new Binary node
    Node* node = make_node(NodeKind::Binary, op, line);
    //hang the left side
    node->left = left_side;
    //hang the right side
    node->right = right_side;
    //give back where it is
    return node;
}

// makes an operator node with one side, like -hp or not alive, and gives back where it is
Node* make_unary_node(string op, Node* side, int line) {
    //a new Unary node
    Node* node = make_node(NodeKind::Unary, op, line);
    //hang its one side on the left
    node->left = side;
    //right stays nullptr
    return node;
}

// turns a node kind into its word, for printing
string node_kind_name(NodeKind kind) {
    switch (kind) {
        case NodeKind::Number:  return "Number";
        case NodeKind::Name:    return "Name";
        case NodeKind::Binary:  return "Binary";
        case NodeKind::Unary:   return "Unary";
        case NodeKind::Error:   return "Error";
        case NodeKind::Assign:  return "Assign";
        case NodeKind::Blank:   return "Blank";
        case NodeKind::Program: return "Program";
        case NodeKind::Declare: return "Declare";
        case NodeKind::Type:    return "Type";
        case NodeKind::Modifier: return "Modifier";
        case NodeKind::Size:    return "Size";
        case NodeKind::If:      return "If";
        case NodeKind::Branch:  return "Branch";
        case NodeKind::While:   return "While";
        case NodeKind::For:     return "For";
        case NodeKind::Parts:   return "Parts";
        case NodeKind::Function: return "Function";
        case NodeKind::Inputs:  return "Inputs";
        case NodeKind::Return:  return "Return";
        case NodeKind::Break:   return "Break";
        case NodeKind::Continue: return "Continue";
        case NodeKind::Call:    return "Call";
        case NodeKind::Field:   return "Field";
        case NodeKind::Index:   return "Index";
        case NodeKind::Follow:  return "Follow";
        case NodeKind::Bool:    return "Bool";
        case NodeKind::String:  return "String";
        case NodeKind::Character: return "Character";
        case NodeKind::List:    return "List";
        case NodeKind::Struct:  return "Struct";
        case NodeKind::Enum:    return "Enum";
        case NodeKind::Class:   return "Class";
        case NodeKind::Access:  return "Access";
        case NodeKind::This:    return "This";
        case NodeKind::Undefined: return "Undefined";
        case NodeKind::Cast:    return "Cast";
        case NodeKind::Choice:  return "Choice";

    }
    return "?";
}

// prints a node and everything under it, each level pushed in 2 more spaces
void print_tree(Node* node, int depth, string label) {
    if (node == nullptr) {
        return;  
    }

    for (int i = 0; i < depth; i++) {
        cout << "  ";
    }
    cout << label << node_kind_name(node->kind) << " " << node->text << endl;

    //print th left side, one level deeper
    print_tree(node->left, depth + 1, "left: ");
    //print the right side, one level deeper
    print_tree(node->right, depth + 1, "right: ");

    //print every child in order, one level down mor
    for (size_t i = 0; i < node->children.size(); i=i+1) {
        print_tree(node->children[i], depth + 1, "child: ");
    }

}

