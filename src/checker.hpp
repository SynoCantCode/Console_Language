// the checker looks at the tree for mistakes in meaning

#pragma once
#include <vector>
#include "ast.hpp"
using namespace std;

// promises
void open_level();
void close_level();
Node* declare_name(string name, Node* node);
Node* find_name(string name);
