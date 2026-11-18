#pragma once
#include <string>
#include <vector>
using namespace std;

// what kind of piece a token is
enum class TokenKind {
    Name,
    Keyword,
    Number,
    String,
    Character,
    Symbol,
    Newline,
    End,
    Error, //a mistake in the code, its text is the message
};

// which keyword it is, only for Keyword tokens and None for everything else
enum class KeywordKind {
    None,
    // blocks
    Fn, Endfn, If, Elif, Else, Endif,
    While, Endwhile, For, Endfor,
    Struct, Endstruct, Enum, Endenum, Bits, Endbits,
    Class, Endclass,
    // classes
    Private, This,
    // leaving
    Return, Break, Continue,
    // logic
    Not, And, Or, True, False,
    // types
    Void, Char, Short, Int, Long, Float, Double, Bool, Unsigned,
    // other
    Const, Import, As, Undefined,
    // hardware
    Load, Store, From, To,
    // game
    Fixed, Str, Vec2, Vec3, Vec4, Mat4,
    Defer, Yield, Asset, Watch, Budget,
};

struct Token {
    TokenKind kind;
    string text;
    int line;
    KeywordKind keyword = KeywordKind::None; //which keyword it is, if kind is Keyword
};

// one keyword, its word and which keyword it is
struct KeywordEntry {
    string text;
    KeywordKind keyword;
};

// the promises, their bodies are in token.cpp
bool is_keyword(const string &word);
KeywordKind keyword_kind(const string &word);
string kind_name(TokenKind kind);
string keyword_name(KeywordKind keyword);
