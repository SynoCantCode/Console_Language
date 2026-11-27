// langc is the compiler for the console's language

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "lexer.hpp"
#include "parser.hpp"
#include "checker.hpp"
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
            cerr << path << ":" << t.line << ": lexer error: " << t.text << endl;
        }
    }
}


//print each mistake the parser or checker found with its file and line and which step found it
void print_stage_errors(const string &path, const vector<Node*> &errors, const string &stage) {
    for (Node* error : errors) {
        cerr << path << ":" << error->line << ": " << stage << " error: " << error->text << endl;
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
    //langc [options] FILE.lang, the file is always the last word, the options go in between
    if (argc < 2) {
        cerr << "usage: langc [--tokens] [--ast] FILE.lang" << endl;
        return 1;
    }

    //the last word is the file
    string path = argv[argc - 1];
    if (path.substr(0, 2) == "--") {
        cerr << "usage: langc [--tokens] [--ast] FILE.lang" << endl;
        return 1;
    }

    //which things to print, all off until an option turns them on
    bool show_tokens = false;
    bool show_tree = false;

    //every word between the program's name and the file is an option
    for (int i = 1; i < argc - 1; i++) {
        string option = argv[i];
        if (option == "--tokens") {
            show_tokens = true;
        } else if (option == "--ast") {
            show_tree = true;
        } else {
            cerr << "langc: unknown option " << option << endl;
            return 1;
        }
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

    //step 3, print the tokens if asked
    if (show_tokens == true) {
        print_tokens(tokens);
    }

    //step 4, read the tokens into a tree, stop if the parser found mistakes
    vector<Node*> errors;
    Node* program = parse(tokens, errors);
    if (errors.size() > 0) {
        print_stage_errors(path, errors, "parser");
        return 1;
    }

    //step 5, print the tree if asked
    if (show_tree == true) {
        print_tree(program, 0, "");
    }

    //step 6, check the tree for mistakes in meaning
    check(program, errors);
    if (errors.size() > 0) {
        print_stage_errors(path, errors, "checker");
        return 1;
    }
    return 0;
}
