#include "token.hpp"


// keywords, each word and which keyword it is

const vector<KeywordEntry> KEYWORDS = {
    // blocks
    { "fn", KeywordKind::Fn }, { "endfn", KeywordKind::Endfn },
    { "if", KeywordKind::If }, { "elif", KeywordKind::Elif }, { "else", KeywordKind::Else }, { "endif", KeywordKind::Endif },
    { "while", KeywordKind::While }, { "endwhile", KeywordKind::Endwhile },
    { "for", KeywordKind::For }, { "endfor", KeywordKind::Endfor },
    { "struct", KeywordKind::Struct }, { "endstruct", KeywordKind::Endstruct },
    { "enum", KeywordKind::Enum }, { "endenum", KeywordKind::Endenum },
    { "bits", KeywordKind::Bits }, { "endbits", KeywordKind::Endbits },
    { "class", KeywordKind::Class }, { "endclass", KeywordKind::Endclass },
    // classes
    { "private", KeywordKind::Private }, { "this", KeywordKind::This },
    // leaving
    { "return", KeywordKind::Return }, { "break", KeywordKind::Break }, { "continue", KeywordKind::Continue },
    // logic
    { "not", KeywordKind::Not }, { "and", KeywordKind::And }, { "or", KeywordKind::Or },
    { "true", KeywordKind::True }, { "false", KeywordKind::False },
    // types
    { "void", KeywordKind::Void }, { "char", KeywordKind::Char }, { "short", KeywordKind::Short },
    { "int", KeywordKind::Int }, { "long", KeywordKind::Long }, { "float", KeywordKind::Float },
    { "double", KeywordKind::Double }, { "bool", KeywordKind::Bool }, { "unsigned", KeywordKind::Unsigned },
    // other
    { "const", KeywordKind::Const }, { "import", KeywordKind::Import }, { "as", KeywordKind::As }, { "undefined", KeywordKind::Undefined },
    // hardware
    { "load", KeywordKind::Load }, { "store", KeywordKind::Store }, { "from", KeywordKind::From }, { "to", KeywordKind::To },
    // game
    { "fixed", KeywordKind::Fixed }, { "str", KeywordKind::Str },
    { "vec2", KeywordKind::Vec2 }, { "vec3", KeywordKind::Vec3 }, { "vec4", KeywordKind::Vec4 }, { "mat4", KeywordKind::Mat4 },
    { "defer", KeywordKind::Defer }, { "yield", KeywordKind::Yield }, { "asset", KeywordKind::Asset },
    { "watch", KeywordKind::Watch }, { "budget", KeywordKind::Budget },
};


// looking things up

//step 1, is it a keyword
bool is_keyword(const string &word) {
    for (const KeywordEntry &entry : KEYWORDS) {
        if (entry.text == word) {
            return true;
        }
    }
    return false;
}

//step 2, which keyword is it, only call it after is_keyword says yes
KeywordKind keyword_kind(const string &word) {
    for (const KeywordEntry &entry : KEYWORDS) {
        if (entry.text == word) {
            return entry.keyword;
        }
    }
    return KeywordKind::None; //not a keyword after all
}


// names for printing

//turns a kind into words
string kind_name(TokenKind kind) {
    switch (kind) {
        case TokenKind::Name:           return "Name";
        case TokenKind::Keyword:        return "Keyword";
        case TokenKind::Number:         return "Number";
        case TokenKind::String:         return "String";
        case TokenKind::Character:      return "Character";
        case TokenKind::Symbol:         return "Symbol";
        case TokenKind::Newline:        return "Newline";
        case TokenKind::End:            return "End";
        case TokenKind::Error:          return "Error";
    }
    return "?";
}

//turns a keyword into its word
string keyword_name(KeywordKind keyword) {
    for (const KeywordEntry &entry : KEYWORDS) {
        if (entry.keyword == keyword) {
            return entry.text;
        }
    }
    return "none";
}
