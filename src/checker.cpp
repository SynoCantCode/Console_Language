#include "checker.hpp"
#include <algorithm>
// who calls who
// check -> check_statement -> check_declare
// check_declare asks type_of for the type of the right side then compares then adds the name with declare_name
// check_statement also hands assigns to check_assign which asks type_of for both sides then compares
// type_of calls find_name whenever its handed a name
// type_of hands a front symbol like -hp to type_of_unary which calls type_of for its one side
// check_declare and check_assign call check_range to see a typed number isnt too big for its spot
// check first adds every functions name to the list then checks each line
// check_function opens a level then adds the inputs as names then hands each inside line to check_statement then closes the level
// check_function asks always_returns whether every path through the function reaches a return
// check_return compares the returned value with what current_function promised
// type_of hands a call like add(1, 2) to type_of_call which finds the function and checks the inputs against its first line
// check_for is the same but opens its level first so its own variable like i lives inside it
// check_if and check_while check the condition then open_level then hand each inside line back to check_statement then close_level
// type_fits asks can this value go into this spot and shared_type asks what one type do two sides share
// type_of hands a binary like hp + 1 to type_of_binary which calls type_of for each side then works out the result from the operator
//im checking it...
// the names the checker knows about

// one name the checker knows about
struct Symbol {
    string name;
    int level;
    Node* node;
};

// every name we can use right now with the newest at the end and also i delete the stuff when tis c losed too.
vector<Symbol> symbols;

// how deep the checker is right now
int checker_level = 0;

// the function the checker is inside right now or nullptr when its not in one
Node* current_function = nullptr;

// going into a block like a function or an if
void open_level() {
    checker_level = checker_level + 1;
}

// leaving a block so its names get thrown away
void close_level() {
    // the newest level's names are always the last ones in the list
    while (symbols.size() > 0 && symbols.back().level == checker_level) { //add the first part for safety bc it could crash
        symbols.pop_back();
    }
    checker_level = checker_level - 1;
}
//im checking it

// adds a name at the level were on and gives back nullptr or an Error if its already here
Node* declare_name(string name, Node* node) {
    // only look at this levels names which are the last ones in the list
    for (int i = symbols.size() - 1; i >= 0; i = i - 1) {
        // a lower level means this levels names are done
        if (symbols[i].level != checker_level) {
            break;
        }
        if (symbols[i].name == name) {
            string message = name + " is already declared on line " + to_string(symbols[i].node->line);
            return make_node(NodeKind::Error, message, node->line);
        }
    }

    // not here yet so add it to the end
    Symbol symbol;
    symbol.name = name;
    symbol.level = checker_level;
    symbol.node = node;
    symbols.push_back(symbol);
    return nullptr;
}

// looks a name up from the newest to the oldest and gives back the node that declared it or nullptr
Node* find_name(string name) {
    for (int i = symbols.size() - 1; i >= 0; i = i - 1) {
        if (symbols[i].name == name) {
            return symbols[i].node;
        }
    }
    // not declared anywhere we can see
    return nullptr;
}

// the mistakes the checker found

vector<Node*> checker_errors;

// records one mistake with its line
void add_error(string message, int line) {
    checker_errors.push_back(make_node(NodeKind::Error, message, line));
}

// types

// plain numbers have no Type node in the tree so the checker keeps its own
Node* int_type = make_node(NodeKind::Type, "int", 0);
Node* float_type = make_node(NodeKind::Type, "float", 0);
Node* bool_type = make_node(NodeKind::Type, "bool", 0);
Node* str_type = make_node(NodeKind::Type, "str", 0);
Node* char_type = make_node(NodeKind::Type, "char", 0);

// are two types the same type
bool same_type(Node* a, Node* b) {
    // int vs float or ^ vs int
    if (a->text != b->text) {
        return false;
    }
    // one is a pointer to something and the other isnt
    if ((a->left == nullptr) != (b->left == nullptr)) {
        return false;
    }
    // both pointers so compare what they point to
    if (a->left != nullptr) {
        return same_type(a->left, b->left);
    }
    return true;
}

// is this type char or short or int or long
bool is_whole_number_type(Node* type) {
    // a pointer isnt a number type
    if (type->left != nullptr) {
        return false;
    }
    string t = type->text;
    return t == "char" || t == "short" || t == "int" || t == "long";
}

// can a value of value_type go into a spot of type target
bool type_fits(Node* target, Node* value_type) {
    if (same_type(target, value_type) == true) {
        return true;
    }
    // a whole number typed in the code fits any whole number type
    if (value_type == int_type && is_whole_number_type(target) == true) {
        return true;
    }
    return false;
}

// the one type the two sides of an operator share or nullptr if they cant mix
Node* shared_type(Node* left, Node* right) {
    // a whole number typed in the code takes the other sides type
    if (left == int_type && is_whole_number_type(right) == true) {
        return right;
    }
    if (right == int_type && is_whole_number_type(left) == true) {
        return left;
    }
    if (same_type(left, right) == true) {
        return left;
    }
    return nullptr;
}

// a type as words for error messages like int or Player^
string type_name(Node* type) {
    if (type->text == "^") {
        return type_name(type->left) + "^";
    }
    return type->text;
}

// values

// promise bc type_of and type_of_binary call each other
Node* type_of(Node* node);

// AI generated
// works out the type of a value with a front symbol like -hp or not alive or &hp
Node* type_of_unary(Node* node) {
    Node* side_type = type_of(node->left);
    // nullptr means the side is unknown and its mistake is already recorded
    if (side_type == nullptr) {
        return nullptr;
    }

    string op = node->text;

    // -hp is the same type as hp and needs a number
    if (op == "-") {
        bool is_number = is_whole_number_type(side_type) == true || side_type->text == "float" || side_type->text == "double";
        if (is_number == false) {
            add_error("- needs a number", node->line);
            return nullptr;
        }
        return side_type;
    }

    // not alive gives true or false and needs true or false
    if (op == "not") {
        if (same_type(side_type, bool_type) == false) {
            add_error("not needs true or false", node->line);
            return nullptr;
        }
        return bool_type;
    }

    // &hp is the address of hp so the side has to be something stored in memory
    NodeKind k = node->left->kind;
    if (k != NodeKind::Name && k != NodeKind::Field && k != NodeKind::Index && k != NodeKind::Follow) {
        add_error("& needs a variable", node->line);
        return nullptr;
    }
    // build the pointer type which is ^ with the sides type under it
    Node* pointer = make_node(NodeKind::Type, "^", node->line);
    pointer->left = side_type;
    return pointer;
}

// promise bc type_of_call uses it and its written further down
void check_range(Node* target, Node* value);

// works out the type of a call like add(1, 2) by checking it against the functions first line
Node* type_of_call(Node* node) {
    // every input gets its own type worked out first which also catches mistakes inside the inputs
    vector<Node*> input_types;
    for (size_t i = 0; i < node->children.size(); i = i + 1) {
        input_types.push_back(type_of(node->children[i]));
    }

    // what is being called
    Node* called = node->left;
    if (called->kind != NodeKind::Name) {
        // calls through a field like enemy.hit() come with classes
        return nullptr;
    }
    Node* function = find_name(called->text);
    if (function == nullptr) {
        add_error(called->text + " is not declared", node->line);
        return nullptr;
    }
    if (function->kind != NodeKind::Function) {
        add_error(called->text + " is not a function", node->line);
        return nullptr;
    }

    // the right number of inputs
    Node* wanted = function->right;
    if (wanted->children.size() != node->children.size()) {
        add_error(called->text + " needs " + to_string(wanted->children.size()) + " inputs but got " + to_string(node->children.size()), node->line);
        return function->left;
    }

    // each input has to fit the spot the function gave it
    for (size_t i = 0; i < node->children.size(); i = i + 1) {
        Node* spot_type = wanted->children[i]->left;
        if (input_types[i] == nullptr) {
            continue;
        }
        if (type_fits(spot_type, input_types[i]) == false) {
            add_error("input " + to_string(i + 1) + " of " + called->text + ": can't put " + type_name(input_types[i]) + " into " + type_name(spot_type), node->line);
        } else {
            check_range(spot_type, node->children[i]);
        }
    }

    // the calls type is what the function promised to give back
    return function->left;
}

// works out the type of a value with an operator in the middle like hp + 1 or hp > 0
Node* type_of_binary(Node* node) {
    Node* left_type = type_of(node->left);
    Node* right_type = type_of(node->right);
    // nullptr means a side is unknown and its mistake is already recorded
    if (left_type == nullptr || right_type == nullptr) {
        return nullptr;
    }

    string op = node->text;

    // and / or need true or false on both sides and give true or false
    if (op == "and" || op == "or") {
        if (same_type(left_type, bool_type) == false || same_type(right_type, bool_type) == false) {
            add_error(op + " needs true or false on both sides", node->line);
            return nullptr;
        }
        return bool_type;
    }

    // every other operator needs one type both sides share
    Node* shared = shared_type(left_type, right_type);
    if (shared == nullptr) {
        add_error("can't mix " + type_name(left_type) + " and " + type_name(right_type), node->line);
        return nullptr;
    }

    // a compare gives true or false
    if (op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">=") {
        return bool_type;
    }

    // true and false arent numbers
    if (same_type(shared, bool_type) == true) {
        add_error("can't do math with bool", node->line);
        return nullptr;
    }

    // joining text is a library function not an operator
    if (same_type(shared, str_type) == true) {
        add_error("can't do math with str", node->line);
        return nullptr;
    }

    // math gives the type its sides share
    return shared;
}

// works out the type of one value and gives back nullptr if it cant be known
// when its handed a name it calls find_name to look it up in the list
Node* type_of(Node* node) {
    if (node->kind == NodeKind::Number) {
        // a dot means its a decimal number
        if (node->text.find('.') != string::npos) {
            return float_type;
        }
        return int_type;
    }

    if (node->kind == NodeKind::Name) {
        Node* found = find_name(node->text);
        if (found == nullptr) {
            add_error(node->text + " is not declared", node->line);
            return nullptr;
        }
        // a functions name by itself isnt a value
        if (found->kind == NodeKind::Function) {
            add_error(node->text + " is a function so it needs ( ) to call it", node->line);
            return nullptr;
        }
        // the type is on the left of the node that declared it
        return found->left;
    }

    // true or false
    if (node->kind == NodeKind::Bool) {
        return bool_type;
    }

    // text in double quotes like "hero"
    if (node->kind == NodeKind::String) {
        return str_type;
    }

    // one character in single quotes like 'a'
    if (node->kind == NodeKind::Character) {
        return char_type;
    }

    if (node->kind == NodeKind::Binary) {
        return type_of_binary(node);
    }

    if (node->kind == NodeKind::Unary) {
        return type_of_unary(node);
    }

    if (node->kind == NodeKind::Call) {
        return type_of_call(node);
    }

    // kinds we havent added yet
    return nullptr;
}

// lines

// AI generated
// records a mistake if a number typed in the code is too big or too small for the spot its going into
void check_range(Node* target, Node* value) {
    // a - in front of a typed number
    bool negative = false;
    Node* number = value;
    if (value->kind == NodeKind::Unary && value->text == "-") {
        negative = true;
        number = value->left;
    }

    // only whole numbers typed in the code going into a whole number type get checked
    if (number->kind != NodeKind::Number || number->text.find('.') != string::npos) {
        return;
    }
    if (is_whole_number_type(target) == false) {
        return;
    }

    // which base its written in like 0xFF or 0b101 or plain 255
    string digits = number->text;
    unsigned long long base = 10;
    if (digits.substr(0, 2) == "0x") {
        base = 16;
        digits = digits.substr(2);
    } else if (digits.substr(0, 2) == "0b") {
        base = 2;
        digits = digits.substr(2);
    }

    // turn the text into a real number one digit at a time
    unsigned long long amount = 0;
    bool too_big = false;
    for (size_t i = 0; i < digits.size(); i = i + 1) {
        char d = digits[i];
        // 1_000 is allowed so skip the _
        if (d == '_') {
            continue;
        }
        unsigned long long digit = 0;
        if (d >= '0' && d <= '9') {
            digit = d - '0';
        } else if (d >= 'a' && d <= 'f') {
            digit = d - 'a' + 10;
        } else {
            digit = d - 'A' + 10;
        }
        // it wouldnt even fit in 8 bytes
        if (amount > (18446744073709551615ULL - digit) / base) {
            too_big = true;
            break;
        }
        amount = amount * base + digit;
    }

    // how many bits the type has and whether its unsigned
    int bits = 32;
    if (target->text == "char") {
        bits = 8;
    } else if (target->text == "short") {
        bits = 16;
    } else if (target->text == "long") {
        bits = 64;
    }
    bool is_unsigned = false;
    for (size_t i = 0; i < target->children.size(); i = i + 1) {
        if (target->children[i]->text == "unsigned") {
            is_unsigned = true;
        }
    }

    // the biggest number the type holds and how far below zero it goes
    unsigned long long highest = 0;
    unsigned long long lowest = 0;
    if (is_unsigned == true) {
        if (bits == 64) {
            highest = 18446744073709551615ULL;
        } else {
            highest = (1ULL << bits) - 1;
        }
    } else {
        highest = (1ULL << (bits - 1)) - 1;
        lowest = 1ULL << (bits - 1);
    }

    bool fits = true;
    if (too_big == true) {
        fits = false;
    } else if (negative == true && amount > lowest) {
        fits = false;
    } else if (negative == false && amount > highest) {
        fits = false;
    }

    if (fits == false) {
        string written = number->text;
        if (negative == true) {
            written = "-" + written;
        }
        string type_words = target->text;
        string low_words = "-" + to_string(lowest);
        if (is_unsigned == true) {
            type_words = "unsigned " + type_words;
            low_words = "0";
        }
        add_error(written + " doesn't fit in " + type_words + " (" + low_words + " to " + to_string(highest) + ")", value->line);
    }
}

// checks a declaration like int hp = 10
void check_declare(Node* node) {
    // the value gets checked before the name exists so int x = x + 1 is a mistake
    if (node->right != nullptr) {
        Node* value_type = type_of(node->right);
        // nullptr means the type is unknown and its mistake is already recorded
        if (value_type != nullptr && type_fits(node->left, value_type) == false) {
            add_error("can't put " + type_name(value_type) + " into " + type_name(node->left), node->line);
        } else if (value_type != nullptr) {
            // the type is right so see if the number itself is small enough
            check_range(node->left, node->right);
        }
    }

    // now the name exists
    Node* problem = declare_name(node->text, node);
    if (problem != nullptr) {
        checker_errors.push_back(problem);
    }
}

// checks an assignment like hp = 5
void check_assign(Node* node) {
    // type_of also catches a name that was never declared
    Node* left_type = type_of(node->left);
    Node* right_type = type_of(node->right);
    // nullptr means the type is unknown and its mistake is already recorded
    if (left_type != nullptr && right_type != nullptr && type_fits(left_type, right_type) == false) {
        add_error("can't put " + type_name(right_type) + " into " + type_name(left_type), node->line);
    } else if (left_type != nullptr && right_type != nullptr) {
        // the type is right so see if the number itself is small enough
        check_range(left_type, node->right);
    }
}

// promise bc the block workers and the boss call each other
void check_statement(Node* node);

// the condition of an if or while has to be true or false
void check_condition(Node* condition) {
    Node* type = type_of(condition);
    // nullptr means the type is unknown and its mistake is already recorded
    if (type != nullptr && same_type(type, bool_type) == false) {
        add_error("the condition needs true or false but this is " + type_name(type), condition->line);
    }
}

// checks a while block
void check_while(Node* node) {
    check_condition(node->left);

    // names made inside only exist inside
    open_level();
    for (size_t i = 0; i < node->children.size(); i = i + 1) {
        check_statement(node->children[i]);
    }
    close_level();
}

// checks an if block which is a list of branches
void check_if(Node* node) {
    for (size_t b = 0; b < node->children.size(); b = b + 1) {
        Node* branch = node->children[b];

        // else has no condition
        if (branch->left != nullptr) {
            check_condition(branch->left);
        }

        // each branch gets its own level so its names dont leak into the next branch
        open_level();
        for (size_t i = 0; i < branch->children.size(); i = i + 1) {
            check_statement(branch->children[i]);
        }
        close_level();
    }
}

// checks a for block
void check_for(Node* node) {
    // the level opens first so the fors own variable like i lands inside it
    open_level();

    // the start like int i = 0 and the step like i = i + 1 are normal lines
    Node* parts = node->right;
    check_statement(parts->left);
    // the check like i < 10 can see i
    check_condition(node->left);
    check_statement(parts->right);

    for (size_t i = 0; i < node->children.size(); i = i + 1) {
        check_statement(node->children[i]);
    }
    close_level();
}

// do these lines reach a return no matter which way the ifs go
bool always_returns(vector<Node*> &lines) {
    for (size_t i = 0; i < lines.size(); i = i + 1) {
        Node* line = lines[i];
        if (line->kind == NodeKind::Return) {
            return true;
        }
        if (line->kind == NodeKind::If) {
            // an if only counts when it has an else and every branch returns
            Node* last_branch = line->children.back();
            bool has_else = last_branch->left == nullptr;
            bool every_branch_returns = true;
            for (size_t b = 0; b < line->children.size(); b = b + 1) {
                if (always_returns(line->children[b]->children) == false) {
                    every_branch_returns = false;
                }
            }
            if (has_else == true && every_branch_returns == true) {
                return true;
            }
        }
    }
    return false;
}

// checks a whole function from its inputs to its last line
void check_function(Node* node) {
    if (checker_level != 0) {
        add_error("a function can't be inside another function or block", node->line);
    }

    // the return lines inside need to know which function theyre in
    current_function = node;
    open_level();

    // the inputs are names the lines inside can use
    Node* inputs = node->right;
    for (size_t i = 0; i < inputs->children.size(); i = i + 1) {
        Node* input = inputs->children[i];
        Node* problem = declare_name(input->text, input);
        if (problem != nullptr) {
            checker_errors.push_back(problem);
        }
    }

    for (size_t i = 0; i < node->children.size(); i = i + 1) {
        check_statement(node->children[i]);
    }

    // a function that gives something back has to return on every path
    Node* promised = node->left;
    bool gives_nothing = promised->text == "void" && promised->left == nullptr;
    if (gives_nothing == false && always_returns(node->children) == false) {
        add_error("this function gives " + type_name(promised) + " but it can end without a return", node->line);
    }

    close_level();
    current_function = nullptr;
}

// checks a return line against what the function its in promised to give back
void check_return(Node* node) {
    if (current_function == nullptr) {
        add_error("return can only be inside a function", node->line);
        return;
    }

    Node* promised = current_function->left;
    bool gives_nothing = promised->text == "void" && promised->left == nullptr;

    // a plain return with no value
    if (node->left == nullptr) {
        if (gives_nothing == false) {
            add_error("this function gives " + type_name(promised) + " so return needs a value", node->line);
        }
        return;
    }

    if (gives_nothing == true) {
        add_error("this function gives nothing so return can't have a value", node->line);
        return;
    }

    Node* value_type = type_of(node->left);
    // nullptr means the type is unknown and its mistake is already recorded
    if (value_type == nullptr) {
        return;
    }
    if (type_fits(promised, value_type) == false) {
        add_error("can't return " + type_name(value_type) + " from a function that gives " + type_name(promised), node->line);
    } else {
        check_range(promised, node->left);
    }
}

// checks one line by handing it to the right worker
void check_statement(Node* node) {
    switch (node->kind) {
        case NodeKind::Declare:
            check_declare(node);
            break;
        case NodeKind::Assign:
            check_assign(node);
            break;
        case NodeKind::If:
            check_if(node);
            break;
        case NodeKind::While:
            check_while(node);
            break;
        case NodeKind::For:
            check_for(node);
            break;
        case NodeKind::Function:
            check_function(node);
            break;
        case NodeKind::Return:
            check_return(node);
            break;
        case NodeKind::Call:
            // a call on its own line like draw(hp) still gets checked and what it gives back is dropped
            type_of(node);
            break;
        default:
            // kinds we havent added yet
            break;
    }
}

// for sorting the mistakes so the earliest line comes first
bool earlier_line(Node* a, Node* b) {
    return a->line < b->line;
}

// the start of the checker which takes the parsers tree and fills errors with the mistakes it finds
void check(Node* program, vector<Node*> &errors) {
    symbols.clear();
    checker_level = 0;
    checker_errors.clear();
    current_function = nullptr;

    // first pass: every functions name goes in the list so they can be called from anywhere in the file
    for (size_t i = 0; i < program->children.size(); i = i + 1) {
        Node* line = program->children[i];
        if (line->kind == NodeKind::Function) {
            Node* problem = declare_name(line->text, line);
            if (problem != nullptr) {
                checker_errors.push_back(problem);
            }
        }
    }

    // second pass: the real check
    for (size_t i = 0; i < program->children.size(); i = i + 1) {
        check_statement(program->children[i]);
    }
    // the first pass can find a mistake on a later line before the second pass finds an earlier one
    stable_sort(checker_errors.begin(), checker_errors.end(), earlier_line);
    errors = checker_errors;
}
