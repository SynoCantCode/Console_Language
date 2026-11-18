#pragma once
#include <string>
#include <vector>
#include "token.hpp"
using namespace std;

// chops the source text into tokens
vector<Token> lex(const string &source);
