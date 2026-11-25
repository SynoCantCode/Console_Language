#include "parser.hpp"

Node* read_NNV(); //promise
// promise bc read_if and the boss call each other
Node* read_statement();
//errors
vector<Node*> errors_found;

// the list of tokens the parser is reading, and the finger
vector<Token> tokens;
size_t position = 0;

// gives back the token the finger is on without moving the finger
Token current_token() {
    return tokens[position];
}

// looks ahead how_far tokens past the finger without moving it
Token token_after(size_t how_far) {
    // past the end just gives back End
    if (position + how_far >= tokens.size()) {
        return tokens.back();
    }
    return tokens[position + how_far];
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

// is the finger on this keyword
bool is_keyword(KeywordKind keyword) {
    Token token = current_token();
    if (token.kind == TokenKind::Keyword && token.keyword == keyword) {
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
Node* expect_line_end(string line_end) {
    if (line_end == "newline") {
        return expect_newline();
    }
    return expect_symbol(line_end);
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
        Node* left_side = read_NNV(); //new name and look same old guy we know and love...
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
Node* read_name_line(string line_end = "newline") {
    // the left side of the =, or the whole line if it is a call
    Node* left_side = read_expression(0);
    if (left_side->kind == NodeKind::Error) {
        return left_side;
    }

    // no = means it is a call line, so the expression is the whole line
    if (is_symbol("=") == false) {
        Node* problem = expect_line_end(line_end);
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

    Node* problem = expect_line_end(line_end);
    if (problem != nullptr) {
        return problem;
    }

    // the Assign node, the name on its left and the value on its right
    Node* assign = make_node(NodeKind::Assign, "=", equals.line);
    assign->left = left_side;
    assign->right = right_side;
    return assign;
}

// is this token one of the 14 type keywords, like int or vec3
bool is_type_keyword(Token token) {
    if (token.kind != TokenKind::Keyword) {
        return false;
    }
    KeywordKind k = token.keyword;
    // the plain types
    if (k == KeywordKind::Void || k == KeywordKind::Char || k == KeywordKind::Short || k == KeywordKind::Int ||
        k == KeywordKind::Long || k == KeywordKind::Float || k == KeywordKind::Double || k == KeywordKind::Bool) {
        return true;
    }
    // the game types
    if (k == KeywordKind::Fixed || k == KeywordKind::Str || k == KeywordKind::Vec2 ||
        k == KeywordKind::Vec3 || k == KeywordKind::Vec4 || k == KeywordKind::Mat4) {
        return true;
    }
    return false;
}

// reads a type, a type keyword like int or a struct's name like Player, and gives back a Type node
Node* read_type() {
    // text gets filled in once we reach the type word
    Node* type = make_node(NodeKind::Type, "", current_token().line);

    // const goes first if its there
    if (is_keyword(KeywordKind::Const) == true) {
        Token word = take();
        type->children.push_back(make_node(NodeKind::Modifier, word.text, word.line));
    }

    // then unsigned if its there
    if (is_keyword(KeywordKind::Unsigned) == true) {
        Token word = take();
        type->children.push_back(make_node(NodeKind::Modifier, word.text, word.line));
    }

    // the type word itself like int or Player
    Token token = current_token();
    if (is_type_keyword(token) == true || token.kind == TokenKind::Name) {
        take();
        type->text = token.text;
        // each ^ makes a new Type on top that points to whats below it
        while (is_symbol("^") == true) {
            Token hat = take();
            Node* pointer = make_node(NodeKind::Type, hat.text, hat.line);
            pointer->left = type;
            type = pointer;
        }
        return type;
    }
    return make_node(NodeKind::Error, "expected a type", token.line);
}

// does the line the finger is on start a declaration, like int hp = 10 or Player hero
bool declaration_starts_here() {
    Token token = current_token();
    // int hp = 10
    if (is_type_keyword(token) == true) {
        return true;
    }
    // const int MAX = 100 or unsigned int score
    if (is_keyword(KeywordKind::Const) == true || is_keyword(KeywordKind::Unsigned) == true) {
        return true;
    }
    // Player hero or Player^ p
    if (token.kind == TokenKind::Name) {
        // skip over any ^ after the name
        size_t how_far = 1;
        while (token_after(how_far).kind == TokenKind::Symbol && token_after(how_far).text == "^") {
            how_far = how_far + 1;
        }
        // a name after that means its a type then a name
        if (token_after(how_far).kind == TokenKind::Name) {
            return true;
        }
    }
    return false;
}

// reads a declaration like int hp = 10, gives back a Declare node with the type on its left and the value on its right
Node* read_declaration(string line_end = "newline") {
    // the type, read_type takes it
    Node* type = read_type();
    if (type->kind == NodeKind::Error) {
        return type;
    }

    // the name being made
    Token name = current_token();
    if (name.kind != TokenKind::Name) {
        return make_node(NodeKind::Error, "expected a name", name.line);
    }
    take();

    // the Declare node with the name in its text
    Node* declare = make_node(NodeKind::Declare, name.text, name.line);
    declare->left = type;

    // array sizes like [10] go in children one Size node each
    while (is_symbol("[") == true) {
        Token bracket = take();
        Node* amount = read_expression(0);
        if (amount->kind == NodeKind::Error) {
            return amount;
        }
        Node* problem = expect_symbol("]");
        if (problem != nullptr) {
            return problem;
        }
        // the Size node holds the amount on its left
        Node* size = make_node(NodeKind::Size, "", bracket.line);
        size->left = amount;
        declare->children.push_back(size);
    }

    // start value if theres an =
    if (is_symbol("=") == true) {
        // step past the =
        take();
        Node* value = read_expression(0);
        if (value->kind == NodeKind::Error) {
            return value;
        }
        declare->right = value;
    }

    // the end of the line
    Node* problem = expect_line_end(line_end);
    if (problem != nullptr) {
        return problem;
    }
    return declare;
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

// reads an if or elif or else line and gives back an empty Branch for its lines
Node* read_branch_line() {
    // the if or elif or else word
    Token word = take();
    Node* branch = make_node(NodeKind::Branch, word.text, word.line);

    // else has no condition
    if (word.keyword != KeywordKind::Else) {
        Node* problem = expect_symbol("(");
        if (problem != nullptr) {
            return problem;
        }
        Node* condition = read_expression(0);
        if (condition->kind == NodeKind::Error) {
            return condition;
        }
        problem = expect_symbol(")");
        if (problem != nullptr) {
            return problem;
        }
        branch->left = condition;
    }

    Node* problem = expect_newline();
    if (problem != nullptr) {
        return problem;
    }
    return branch;
}

// reads a whole if block from the if line to the endif line
Node* read_if() {
    Token if_word = current_token();
    Node* if_node = make_node(NodeKind::If, "if", if_word.line);

    // the first branch from the if line
    Node* branch = read_branch_line();
    if (branch->kind == NodeKind::Error) {
        return branch;
    }
    if_node->children.push_back(branch);

    // nothing can come after an else
    bool seen_else = false;

    // the inside lines until endif
    while (is_keyword(KeywordKind::Endif) == false) {
        // the file ended with no endif
        if (current_token().kind == TokenKind::End) {
            return make_node(NodeKind::Error, "this if has no endif", if_word.line);
        }

        if (is_keyword(KeywordKind::Elif) == true || is_keyword(KeywordKind::Else) == true) {
            // elif or else starts a new branch
            if (seen_else == true) {
                return make_node(NodeKind::Error, "nothing can come after else", current_token().line);
            }
            if (is_keyword(KeywordKind::Else) == true) {
                seen_else = true;
            }
            branch = read_branch_line();
            if (branch->kind == NodeKind::Error) {
                return branch;
            }
            if_node->children.push_back(branch);
        } else {
            // any other line goes in the branch were on
            Node* line = read_statement();
            branch->children.push_back(line);
        }
    }

    // the endif line
    take();
    Node* problem = expect_newline();
    if (problem != nullptr) {
        return problem;
    }
    return if_node;
}

// reads a whole while block from the while line to the endwhile line
Node* read_while() {
    // the while word
    Token while_word = take();

    // the condition in brackets
    Node* problem = expect_symbol("(");
    if (problem != nullptr) {
        return problem;
    }
    Node* condition = read_expression(0);
    if (condition->kind == NodeKind::Error) {
        return condition;
    }
    problem = expect_symbol(")");
    if (problem != nullptr) {
        return problem;
    }
    problem = expect_newline();
    if (problem != nullptr) {
        return problem;
    }

    // the While node with the condition on its left
    Node* while_node = make_node(NodeKind::While, "while", while_word.line);
    while_node->left = condition;

    // the inside lines until endwhile
    while (is_keyword(KeywordKind::Endwhile) == false) {
        // the file ended with no endwhile
        if (current_token().kind == TokenKind::End) {
            return make_node(NodeKind::Error, "this while has no endwhile", while_word.line);
        }
        Node* line = read_statement();
        while_node->children.push_back(line);
    }

    // the endwhile line
    take();
    problem = expect_newline();
    if (problem != nullptr) {
        return problem;
    }
    return while_node;
}

// reads a whole for block from the for line to the endfor line
Node* read_for() {
    // the for word
    Token for_word = take();
    Node* problem = expect_symbol("(");
    if (problem != nullptr) {
        return problem;
    }

    // the start like int i = 0 ends at ;
    Node* start = read_declaration(";");
    if (start->kind == NodeKind::Error) {
        return start;
    }

    // the check like i < 10
    Node* check = read_expression(0);
    if (check->kind == NodeKind::Error) {
        return check;
    }
    problem = expect_symbol(";");
    if (problem != nullptr) {
        return problem;
    }

    // the step like i = i + 1 ends at )
    Node* step = read_name_line(")");
    if (step->kind == NodeKind::Error) {
        return step;
    }
    problem = expect_newline();
    if (problem != nullptr) {
        return problem;
    }

    // Parts holds the start and the step
    Node* parts = make_node(NodeKind::Parts, "", for_word.line);
    parts->left = start;
    parts->right = step;

    // the For node with the check on its left like while
    Node* for_node = make_node(NodeKind::For, "for", for_word.line);
    for_node->left = check;
    for_node->right = parts;

    // the inside lines until endfor
    while (is_keyword(KeywordKind::Endfor) == false) {
        // the file ended with no endfor
        if (current_token().kind == TokenKind::End) {
            return make_node(NodeKind::Error, "this for has no endfor", for_word.line);
        }
        Node* line = read_statement();
        for_node->children.push_back(line);
    }

    // the endfor line
    take();
    problem = expect_newline();
    if (problem != nullptr) {
        return problem;
    }
    return for_node;
}

// the boss, looks at the first token of a line and hands the line to the right worker
Node* read_statement() {
    Token token = current_token();
    Node* statement = nullptr;

    if (token.kind == TokenKind::Newline) {
        statement = read_blank_line();
    } else if (is_keyword(KeywordKind::If) == true) {
        statement = read_if();
    } else if (is_keyword(KeywordKind::While) == true) {
        statement = read_while();
    } else if (is_keyword(KeywordKind::For) == true) {
        statement = read_for();
    } else if (declaration_starts_here() == true) {
        statement = read_declaration();
    } else if (token.kind == TokenKind::Name) {
        statement = read_name_line();
    } else {
        statement = make_node(NodeKind::Error, "a line can't start with this", token.line);
    }

    //a mistake leaves the finger in the middle of the line, so record it and skip the rest of the line
    if (statement->kind == NodeKind::Error) {
        errors_found.push_back(statement);
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
Node* parse(vector<Token> lexer_tokens, vector<Node*> &errors) {
    tokens = lexer_tokens;
    position = 0;
    errors_found.clear();
    Node* program = read_program();
    errors = errors_found;
    return program;
}

Node* read_NNV() { //name number value it stands for bc its too long to type
    Token token = current_token();

    //a - or not where a value should start goes on just the next value
    if (is_symbol("-") == true || (token.kind == TokenKind::Keyword && token.keyword == KeywordKind::Not)) {
        //step past the - or not
        take();
        //the one value it goes on, which can have its own - or brackets
        Node* side = read_NNV();
        if (side->kind == NodeKind::Error) {
            return side;
        }
        return make_unary_node(token.text, side, token.line);
    }

    if (is_symbol("(") == true) {
        //step past the (
        take();
        //the inside is a fresh whole expression, so every operator counts again
        Node* inside = read_expression(0);
        if (inside->kind == NodeKind::Error) {
            return inside;
        }
        Node* problem = expect_symbol(")");
        if (problem != nullptr) {
            return problem;
        }
        //the inside's tree counts as one value
        return inside;
    }
    return read_number_or_name();
}
