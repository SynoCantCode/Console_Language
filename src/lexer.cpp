// lexer.cpp chops the source text into tokens
//
// The lexer walks through the text one character at a time, using i as its finger.
// At each spot it checks a list of rules in order. Each rule has its own function
// below, and it moves i past whatever it read.

#include "lexer.hpp"

// symbols, the two character ones go first so a double equals isn't read as two single equals
const vector<string> SYMBOLS = {
    // two characters
    "==", "!=", "<=", ">=", "++", "--", "+=", "-=", "*=", "/=", "..", "::",
    // one character
    "=", "+", "-", "*", "/", "%", "<", ">",
    "(", ")", "[", "]", "{", "}",
    ",", ".", ":", ";", "^", "&",
};


// questions about one character

//is it a space, a tab, or the carriage return Windows puts before each new line
bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r';
}

//is it a letter or an underscore
bool is_letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

//is it a digit
bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

//is it a hex digit
bool is_hex_digit(char c) {
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

//is it a binary digit
bool is_binary_digit(char c) {
    return c == '0' || c == '1';
}


// questions about the text at position i
// each one checks that i is still inside the text first so it never reads past the end

//is i still inside the text, false once the finger walks off the end
bool inside(const string &source, size_t i) {
    return i < source.size();
}

//is this exact character at i
bool character_at(const string &source, size_t i, char wanted) {
    if (inside(source, i) == false) {
        //off the end so nothing is here
        return false;
    }
    return source[i] == wanted;
}

//does this text start here
bool text_at(const string &source, size_t i, const string &text) {
    //cut out as many characters as text has
    string piece = source.substr(i, text.size());
    return piece == text;
}

//which symbol starts here, empty if none
string symbol_at(const string &source, size_t i) {
    for (const string &symbol : SYMBOLS) {
        if (text_at(source, i, symbol) == true) {
            return symbol;
        }
    }
    return "";
}

//does any symbol start here
bool symbol_starts_at(const string &source, size_t i) {
    string symbol = symbol_at(source, i);
    return symbol != "";
}

//does a comment start here
bool comment_starts_at(const string &source, size_t i) {
    bool first_slash = character_at(source, i, '/');
    bool second_slash = character_at(source, i + 1, '/');
    return first_slash && second_slash;
}

//does a hex or binary number start here, the letter is x or b
bool number_prefix_at(const string &source, size_t i, char letter) {
    bool zero = character_at(source, i, '0');
    bool has_letter = character_at(source, i + 1, letter);
    return zero && has_letter;
}

//is i still on this line, inside the text and not at the new line
bool line_continues_at(const string &source, size_t i) {
    if (inside(source, i) == false) {
        return false;
    }
    return source[i] != '\n';
}

//does the word keep going at i, letters digits and underscores
bool word_continues_at(const string &source, size_t i) {
    if (inside(source, i) == false) {
        return false;
    }
    char c = source[i];
    return is_letter(c) || is_digit(c);
}

//do the digits keep going at i, digits and underscores
bool decimal_continues_at(const string &source, size_t i) {
    if (inside(source, i) == false) {
        return false;
    }
    char c = source[i];
    return is_digit(c) || c == '_';
}

//do the hex digits keep going at i, hex digits and underscores
bool hex_continues_at(const string &source, size_t i) {
    if (inside(source, i) == false) {
        return false;
    }
    char c = source[i];
    return is_hex_digit(c) || c == '_';
}

//do the binary digits keep going at i, zeros ones and underscores
bool binary_continues_at(const string &source, size_t i) {
    if (inside(source, i) == false) {
        return false;
    }
    char c = source[i];
    return is_binary_digit(c) || c == '_';
}

//is there a decimal point here with a digit after it
bool decimal_point_at(const string &source, size_t i) {
    if (character_at(source, i, '.') == false) {
        return false;
    }
    if (inside(source, i + 1) == false) {
        //the point is the last character
        return false;
    }
    char after = source[i + 1];
    return is_digit(after);
}

//are we still between the quotes, not at the closing quote and still on this line
bool quoted_continues_at(const string &source, size_t i, char quote) {
    if (line_continues_at(source, i) == false) {
        return false;
    }
    return source[i] != quote;
}

//is there an escape here, a backslash with a character after it on the same line
bool escape_at(const string &source, size_t i) {
    if (character_at(source, i, '\\') == false) {
        return false;
    }
    return line_continues_at(source, i + 1);
}


// rules, each one reads something at i and moves i past it
// the i they get is lex's own i, not a copy

//new line, makes a Newline token
Token read_newline(size_t &i, int line) {
    //step past the new line
    i++;
    Token token = { TokenKind::Newline, "", line };
    return token;
}

//comment, skips to the end of the line and makes no token
void skip_comment(const string &source, size_t &i) {
    while (line_continues_at(source, i) == true) {
        i++;
    }
}

//word, makes a Keyword or Name token
Token read_word(const string &source, size_t &i, int line) {
    //where the word begins
    size_t start = i;
    while (word_continues_at(source, i) == true) {
        i++;
    }
    size_t length = i - start;
    //cut it out
    string word = source.substr(start, length);

    //step 1, is it a keyword
    if (is_keyword(word) == true) {
        //step 2, which keyword it is
        KeywordKind which = keyword_kind(word);
        Token token = { TokenKind::Keyword, word, line, which };
        return token;
    }
    Token token = { TokenKind::Name, word, line };
    return token;
}

//number, makes a Number token for decimal, hex, and binary numbers
Token read_number(const string &source, size_t &i, int line) {
    //where the number begins
    size_t start = i;

    if (number_prefix_at(source, i, 'x') == true) {
        //step past the 0x
        i += 2;
        while (hex_continues_at(source, i) == true) {
            i++;
        }
    } else if (number_prefix_at(source, i, 'b') == true) {
        //step past the 0b
        i += 2;
        while (binary_continues_at(source, i) == true) {
            i++;
        }
    } else {
        while (decimal_continues_at(source, i) == true) {
            i++;
        }
        //decimal point, only if a digit comes after it
        if (decimal_point_at(source, i) == true) {
            //step past the point
            i++;
            while (decimal_continues_at(source, i) == true) {
                i++;
            }
        }
    }

    size_t length = i - start;
    //cut it out
    string number = source.substr(start, length);
    Token token = { TokenKind::Number, number, line };
    return token;
}

//symbol, makes a Symbol token, only call it after symbol_starts_at says yes
Token read_symbol(const string &source, size_t &i, int line) {
    string symbol = symbol_at(source, i);
    //one or two characters
    size_t length = symbol.size();
    //step past it
    i += length;
    Token token = { TokenKind::Symbol, symbol, line };
    return token;
}

//string, makes a String token from the text between the quotes
Token read_string(const string &source, size_t &i, int line) {
    //step past the opening quote
    i++;
    size_t start = i;
    while (quoted_continues_at(source, i, '"') == true) {
        if (escape_at(source, i) == true) {
            //an escape is two characters, step past both so an escaped quote doesn't end the string
            i += 2;
        } else {
            i++;
        }
    }

    if (character_at(source, i, '"') == false) {
        Token error = { TokenKind::Error, "string never closed (missing \")", line };
        return error;
    }

    size_t length = i - start;
    //cut out what's between the quotes
    string text = source.substr(start, length);
    //step past the closing quote
    i++;
    Token token = { TokenKind::String, text, line };
    return token;
}

//bad character, skips the rest of it up to its closing quote if that is on this line
void skip_bad_character(const string &source, size_t &i) {
    while (quoted_continues_at(source, i, '\'') == true) {
        i++;
    }
    if (character_at(source, i, '\'') == true) {
        //step past the closing quote
        i++;
    }
}

//character, makes a Character token from the text between the quotes
Token read_character(const string &source, size_t &i, int line) {
    //step past the opening quote
    i++;
    size_t start = i;
    if (character_at(source, i, '\\') == true) {
        //an escape is two characters
        i++;
    }
    //the character itself
    i++;

    if (character_at(source, i, '\'') == false) {
        skip_bad_character(source, i);
        Token error = { TokenKind::Error, "a character must be one character between ' ', like 'a'", line };
        return error;
    }

    size_t length = i - start;
    //cut out what's between the quotes
    string text = source.substr(start, length);
    //step past the closing quote
    i++;
    Token token = { TokenKind::Character, text, line };
    return token;
}

//a character that means nothing, makes an Error token
Token read_unknown(const string &source, size_t &i, int line) {
    //the one character
    string bad = source.substr(i, 1);
    string message = "unknown character: " + bad;
    //step past it
    i++;
    Token error = { TokenKind::Error, message, line };
    return error;
}


// the lexer

vector<Token> lex(const string &source) {
    //list we fill in, empty at first
    vector<Token> tokens;
    //line number of file we are on
    int line = 1;
    //our position in source
    size_t i = 0;

    while (i < source.size()) {
        char c = source[i];

        if (c == '\n') {
            Token token = read_newline(i, line);
            tokens.push_back(token);
            line++;
        } else if (comment_starts_at(source, i) == true) {
            skip_comment(source, i);
        } else if (is_letter(c) == true) {
            Token token = read_word(source, i, line);
            tokens.push_back(token);
        } else if (is_digit(c) == true) {
            Token token = read_number(source, i, line);
            tokens.push_back(token);
        } else if (c == '"') {
            Token token = read_string(source, i, line);
            tokens.push_back(token);
        } else if (c == '\'') {
            Token token = read_character(source, i, line);
            tokens.push_back(token);
        } else if (symbol_starts_at(source, i) == true) {
            Token token = read_symbol(source, i, line);
            tokens.push_back(token);
        } else if (is_space(c) == true) {
            //spaces between things get skipped
            i++;
        } else {
            Token token = read_unknown(source, i, line);
            tokens.push_back(token);
        }
    }

    Token end = { TokenKind::End, "", line };
    tokens.push_back(end);
    return tokens;
}
