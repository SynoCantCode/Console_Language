// langc is the compiler for the console's language

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "lexer.hpp"
#include "parser.hpp"
using namespace std;

// reads the whole file at path into text, gives back false if the file can't be opened
bool read_file(const string &path, string &text) {
    //open the file for reading
    ifstream file(path);
    if (file.is_open() == false) {
        //it didn't open
        return false;
    }
    //a string we can pour things into
    stringstream buffer;
    //pour the whole file into it
    buffer << file.rdbuf();
    //take the finished string out
    text = buffer.str();
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
    //langc FILE.lang prints the tokens, langc --ast FILE.lang prints the tree
    bool show_tree = false;
    string path;
    if (argc == 2) {
        path = argv[1];
    } else if (argc == 3 && string(argv[1]) == "--ast") {
        show_tree = true;
        path = argv[2];
    } else {
        cerr << "usage: langc [--ast] FILE.lang" << endl;
        return 1;
    }

    string source;
    //fills source with the file's text
    bool opened = read_file(path, source);
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

    //step 3, no mistakes, so print the tree or the tokens
    if (show_tree == true) {
        Node* program = parse(tokens);
        print_tree(program, 0);
        return 0;
    }
    print_tokens(tokens);
    return 0;
}
