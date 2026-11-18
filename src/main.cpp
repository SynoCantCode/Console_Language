// langc is the compiler for the console's language

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "token.hpp"
#include "lexer.hpp"
using namespace std;

// reads the whole file at path into text, gives back false if the file can't be opened
bool read_file(const string &path, string &text) {
    ifstream file(path); //open the file for reading
    if (file.is_open() == false) {
        return false; //it didn't open
    }
    stringstream buffer;   //a string we can pour things into
    buffer << file.rdbuf(); //pour the whole file into it
    text = buffer.str();    //take the finished string out
    return true;
}


// errors

//did the lexer find any mistakes
bool has_error(const vector<Token> &tokens) {
    for (const Token &t : tokens) {
        if (t.kind == TokenKind::Error) {
            return true;
        }
    }
    return false;
}

//print each mistake with its file and line
void print_errors(const string &path, const vector<Token> &tokens) {
    for (const Token &t : tokens) {
        if (t.kind == TokenKind::Error) {
            cerr << path << ":" << t.line << ": " << t.text << endl;
        }
    }
}


// printing

//print one token, its line, kind, which keyword, and text
void print_token(const Token &t) {
    string kind = kind_name(t.kind);
    cout << t.line << "\t" << kind;
    if (t.kind == TokenKind::Keyword) {
        string which = keyword_name(t.keyword);
        cout << " " << which;
    }
    cout << "\t" << t.text << endl;
}

//print every token in the list
void print_tokens(const vector<Token> &tokens) {
    for (const Token &t : tokens) {
        print_token(t);
    }
}


// the compiler

int main(int argc, char **argv) {
    if (argc != 2) {
        cerr << "usage: langc FILE.lang" << endl;
        return 1;
    }

    string path = argv[1];
    string source;
    bool opened = read_file(path, source); //fills source with the file's text
    if (opened == false) {
        cerr << "langc: can't open " << path << endl;
        return 1;
    }

    //step 1, chop the file into tokens
    vector<Token> tokens = lex(source);

    //step 2, stop if there were mistakes, print each one and exit with 1
    if (has_error(tokens) == true) {
        print_errors(path, tokens);
        return 1;
    }

    //step 3, no mistakes so print the tokens
    print_tokens(tokens);
    return 0;
}
