//Abstract Syntax Tree

#pragma once
#include <string>
#include <vector>
using namespace std;

//what kind of node it is

enum class NodeKind {
    Number,
    Name,
    Binary,
    Unary,
    Error,
    Assign,
    Blank,
    Program,
    Declare,
    Type,
    Modifier,
    Size,
    If,
    Branch,
    While,
    For,
    Parts,
    Function,
    Inputs,
    Return,
    Break,
    Continue,
    Call,
    Field,
    Index,
    Follow,
    Bool,
    String,
    Character,
    List,
    Struct,
    Enum,
    Class,
    Access,
    This,
    Undefined,
    Cast,
    Choice,
};

//one node of the tree
struct Node {
    NodeKind kind;
    string text;
    int line;
    Node* left = nullptr;
    Node* right = nullptr;
    vector<Node*> children;
};

//promises, the bodies are in ast.cpp
Node* make_node(NodeKind kind, string text, int line);
Node* make_binary_node(string op, Node* left_side, Node* right_side, int line);
Node* make_unary_node(string op, Node* side, int line);
string node_kind_name(NodeKind kind);
void print_tree(Node* node, int depth, string label);