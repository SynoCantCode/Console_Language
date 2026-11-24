#include "parser.hpp"

// the list of tokens the parser is reading, and the finger
vector<Token> tokens;
size_t position = 0;

// gives back the token the finger is on without moving the finger
Token current_token() {
    return tokens[position];
}

// gives back the token the finger is on, then moves the finger forward one
Token take() {
    //remember the token under the finger
    Token token = tokens[position];
    if (token.kind != TokenKind::End) {
        //move forward, but never past the End token
        position = position + 1;
    }
    //give back the one we were on
    return token;
}

// is the finger on this symbol
bool is_symbol(string text) {
    Token token = current_token();
    if (token.kind == TokenKind::Symbol && token.text == text) {
        return true;
    }
    return false;
}

// the finger must be on this symbol
Node* expect_symbol(string text) {
    if (is_symbol(text) == true) {
        //it's the right one, step past it
        take();
        //no problem
        return nullptr;
    }
    Token token = current_token();
    string message = "expected " + text;
    return make_node(NodeKind::Error, message, token.line);
}

// the finger must be on a new line, takes it and gives back nullptr, or gives back an Error node if it isn't
Node* expect_newline() {
    Token token = current_token();
    if (token.kind == TokenKind::Newline) {
        take();
        return nullptr;
    }
    return make_node(NodeKind::Error, "expected the end of the line", token.line);
}

// a line normally ends at a new line, but inside a for each part counts as its own line, the first two parts end at ; and the last one ends at )
// the finger must be on the end a worker was told to look for, a new line, or a symbol like ; or )
Node* expect_line_end(string end_token) {
    if (end_token == "newline") {
        return expect_newline();
    }
    return expect_symbol(end_token);
}



// reads one number or one name and gives back its node
Node* read_number_or_name() {
    //look at the token we're up to
    Token token = current_token();

    if (token.kind == TokenKind::Number) {
        //step past it
        take();
        //a Number node, like 2
        return make_node(NodeKind::Number, token.text, token.line);
    }

    if (token.kind == TokenKind::Name) {
        //step past it
        take();
        //a Name node, like hp
        return make_node(NodeKind::Name, token.text, token.line);
    }

    //anything else is a mistake here
    return make_node(NodeKind::Error, "expected a value", token.line);
}

// how strong an operator is, 0 means the token is not an operator
int operator_strength(Token token) {
    if (token.kind == TokenKind::Keyword && token.keyword == KeywordKind::Or) {
        return 1;
    }
    if (token.kind == TokenKind::Keyword && token.keyword == KeywordKind::And) {
        return 2;
    }
    if (token.kind != TokenKind::Symbol) {
        //a name, number, newline and so on, not an operator
        return 0;
    }

    string s = token.text;
    if (s == "==" || s == "!=" || s == "<" || s == "<=" || s == ">" || s == ">=") {
        return 3;
    }
    if (s == "+" || s == "-") {
        return 4;
    }
    if (s == "*" || s == "/" || s == "%") {
        return 5;
    }
    //a symbol, but not an operator, like ) or =
    return 0;
}

// reads a whole value with operators, like hp - 2 * 3, and gives back the top node of its tree
Node* read_expression(int limit) {
    //the first value
    Node* left_side = read_number_or_name();
    if (left_side->kind == NodeKind::Error) {
        //pass the mistake up
        return left_side;
    }

    while (true) {
        Token next = current_token();
        int strength = operator_strength(next);
        if (strength <= limit) {
            //not an operator, or not strong enough, so stop
            break;
        }

        //step past the operator
        Token op = take();
        //its right side, only stronger operators join it
        Node* right_side = read_expression(strength);
        if (right_side->kind == NodeKind::Error) {
            //pass the mistake up
            return right_side;
        }

        //new node on top, old tree on its left
        left_side = make_binary_node(op.text, left_side, right_side, op.line);
    }

    //the top of the tree
    return left_side;
}

// reads a line that starts with a name, an assignment like hp = 3 or a call line
Node* read_name_line(string end_token = "newline") {
    // the left side of the =, or the whole line if it is a call
    Node* left_side = read_expression(0);
    if (left_side->kind == NodeKind::Error) {
        return left_side;
    }

    // no = means it is a call line, so the expression is the whole line
    if (is_symbol("=") == false) {
        Node* problem = expect_line_end(end_token);
        if (problem != nullptr) {
            return problem;
        }
        return left_side;
    }

    // only something that can hold a value can go on the left of =
    if (left_side->kind != NodeKind::Name) {
        return make_node(NodeKind::Error, "the left side of = must be a name", left_side->line);
    }

    // step past the =
    Token equals = take();

    // the value being put in
    Node* right_side = read_expression(0);
    if (right_side->kind == NodeKind::Error) {
        return right_side;
    }

    // the end of the line

    Node* problem = expect_line_end(end_token);
    if (problem != nullptr) {
        return problem;
    }

    // the Assign node, the name on its left and the value on its right
    Node* assign = make_node(NodeKind::Assign, "=", equals.line);
    assign->left = left_side;
    assign->right = right_side;
    return assign;
}

// a line with nothing on it, gives back a Blank node that the later steps skip
Node* read_blank_line() {
    //remember it for its line number
    Token token = current_token();
    //take the newline token
    Node* problem = expect_newline();
    if (problem != nullptr) {
        return problem;
    }
    return make_node(NodeKind::Blank, "", token.line);
}

// after a mistake, moves the finger past the rest of the line so the next line can be read
void skip_to_next_line() {
    while (current_token().kind != TokenKind::Newline && current_token().kind != TokenKind::End) {
        take();
    }
    if (current_token().kind == TokenKind::Newline) {
        take();
    }
}

// the boss, looks at the first token of a line and hands the line to the right worker
Node* read_statement() {
    Token token = current_token();
    Node* statement = nullptr;

    if (token.kind == TokenKind::Newline) {
        statement = read_blank_line();
    } else if (token.kind == TokenKind::Name) {
        statement = read_name_line();
    } else {
        statement = make_node(NodeKind::Error, "a line can't start with this", token.line);
    }

    //a mistake leaves the finger in the middle of the line, so skip the rest of it
    if (statement->kind == NodeKind::Error) {
        skip_to_next_line();
    }
    return statement;
}


// reads the whole file one statement at a time, gives back the Program node that holds them all in order
Node* read_program() {
    Node* program = make_node(NodeKind::Program, "", 1);
    while (current_token().kind != TokenKind::End) {
        Node* statement = read_statement();
        program->children.push_back(statement);
    }
    return program;
}

// the start of the parser, takes the lexer's tokens and gives back the Program node for the whole file
Node* parse(vector<Token> lexer_tokens) {
    tokens = lexer_tokens;
    position = 0;
    return read_program();
}
