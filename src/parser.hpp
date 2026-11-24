#pragma once
#include <vector>
#include "token.hpp"
#include "ast.hpp"
using namespace std;

//promises
Node* parse(vector<Token> lexer_tokens, vector<Node*> &errors);
