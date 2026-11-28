#include "checker.hpp"
#include <algorithm>
// how the checker works
//
// the list
//   one list of the names we can use right now. each one has a name and a level and a pointer to its node in the tree
//   the tree holds all the details and never changes. the list only points into it
//
// two passes
//   pass 1 adds the name of every function and struct and enum and class at the top of the file so their order doesnt matter
//   pass 2 walks the file line by line and does the real checking
//
// lines and values
//   check_ functions handle a line and give nothing back. check_statement is the boss that picks which one
//   type_of functions handle a value and give back its type. type_of is the boss that picks which one
//   a type_of that gives back nullptr means the type is unknown and the mistake is already recorded
//
// levels
//   a block opens a level going in and closes it coming out which deletes the names made inside
//
// what the checker keeps track of as it walks
//   checker_level is how deep we are
//   current_function is the function were inside which return lines need
//   current_class is the class were inside which this and private need
//   loops_deep is how many loops were inside which break and continue need
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

// how many while or for loops the checker is inside right now
int loops_deep = 0;

// the class the checker is inside right now or nullptr when its not in one
Node* current_class = nullptr;

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
// what .len gives back which is a plain int and not a typed number
Node* length_type = make_node(NodeKind::Type, "int", 0);

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

// is this type a number which means a whole number type or float or double
bool is_number_type(Node* type) {
    if (is_whole_number_type(type) == true) {
        return true;
    }
    return type->left == nullptr && (type->text == "float" || type->text == "double");
}

// was this type written with const in front
bool has_const(Node* type) {
    for (size_t i = 0; i < type->children.size(); i = i + 1) {
        if (type->children[i]->text == "const") {
            return true;
        }
    }
    return false;
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
    if (type->text == "[]") {
        return type_name(type->left) + "[]";
    }
    return type->text;
}

// is the type written here a real type and if not record the mistake
void check_type(Node* type) {
    // a pointer is real if what it points to is real
    if (type->text == "^") {
        check_type(type->left);
        return;
    }

    // the type words that come with the language
    string t = type->text;
    if (t == "void" || t == "char" || t == "short" || t == "int" || t == "long" || t == "float" || t == "double" ||
        t == "bool" || t == "fixed" || t == "str" || t == "vec2" || t == "vec3" || t == "vec4" || t == "mat4") {
        return;
    }

    // anything else has to be a struct or an enum or a class in the list
    Node* found = find_name(t);
    if (found == nullptr || (found->kind != NodeKind::Struct && found->kind != NodeKind::Enum && found->kind != NodeKind::Class)) {
        add_error(t + " is not a type", type->line);
    }
}

// the full type of a declared variable or field which for int scores[10] is an array of int
Node* declared_type(Node* declare) {
    Node* type = declare->left;
    // each size wraps the type in an array type with the last size innermost
    for (int i = declare->children.size() - 1; i >= 0; i = i - 1) {
        Node* array = make_node(NodeKind::Type, "[]", declare->line);
        array->left = type;
        // the size is kept on the right so an item number can be checked against it
        array->right = declare->children[i]->left;
        type = array;
    }
    return type;
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

// finds a member of a class by its name and looks in the parents too and gives back its Access node or nullptr
Node* find_member(Node* class_node, string name) {
    // the steps limit stops two classes that name each other as parent from looping forever
    for (int steps = 0; steps < 50 && class_node != nullptr; steps = steps + 1) {
        for (size_t i = 0; i < class_node->children.size(); i = i + 1) {
            Node* access = class_node->children[i];
            if (access->left->text == name) {
                return access;
            }
        }
        // not in this class so try its parent
        if (class_node->left == nullptr) {
            return nullptr;
        }
        Node* parent = find_name(class_node->left->text);
        if (parent == nullptr || parent->kind != NodeKind::Class) {
            return nullptr;
        }
        class_node = parent;
    }
    return nullptr;
}

// a private member can only be used by the functions of the class its written in
bool can_use_member(Node* access) {
    if (access->text != "private") {
        return true;
    }
    if (current_class == nullptr) {
        return false;
    }
    for (size_t i = 0; i < current_class->children.size(); i = i + 1) {
        if (current_class->children[i] == access) {
            return true;
        }
    }
    return false;
}

// works out the type of an enums choice like State::Walk which is the enum itself
Node* type_of_choice(Node* node) {
    // the thing before the :: has to be an enums name
    Node* named = nullptr;
    if (node->left->kind == NodeKind::Name) {
        named = find_name(node->left->text);
    }
    if (named == nullptr || named->kind != NodeKind::Enum) {
        add_error(":: needs an enum's name in front of it", node->line);
        return nullptr;
    }

    // look through the enums choices for this name
    for (size_t i = 0; i < named->children.size(); i = i + 1) {
        if (named->children[i]->text == node->text) {
            return make_node(NodeKind::Type, named->text, node->line);
        }
    }
    add_error(named->text + " has no choice called " + node->text, node->line);
    return nullptr;
}

// works out the type of a field like hero.hp by looking inside the struct
Node* type_of_field(Node* node) {
    // an enums choice is written with :: so a dot on an enums name is a mistake
    if (node->left->kind == NodeKind::Name) {
        Node* named = find_name(node->left->text);
        if (named != nullptr && named->kind == NodeKind::Enum) {
            add_error("an enum's choice is written " + named->text + "::" + node->text, node->line);
            return nullptr;
        }
    }

    Node* owner_type = type_of(node->left);
    // nullptr means the left side is unknown and its mistake is already recorded
    if (owner_type == nullptr) {
        return nullptr;
    }

    // the dot works through a pointer too so p.hp is fine when p is a Player^
    if (owner_type->text == "^") {
        owner_type = owner_type->left;
    }

    // arrays and strings have one built in field which is how many items they hold
    if (owner_type->text == "[]" || same_type(owner_type, str_type) == true) {
        if (node->text == "len") {
            return length_type;
        }
        add_error(type_name(owner_type) + " only has len", node->line);
        return nullptr;
    }

    // the left sides type has to be a struct or a class
    Node* struct_node = find_name(owner_type->text);
    if (owner_type->text == "^" || struct_node == nullptr || (struct_node->kind != NodeKind::Struct && struct_node->kind != NodeKind::Class)) {
        add_error(type_name(owner_type) + " has no fields", node->line);
        return nullptr;
    }

    // a classs members are looked up through its parents and can be private
    if (struct_node->kind == NodeKind::Class) {
        Node* access = find_member(struct_node, node->text);
        if (access == nullptr) {
            add_error(struct_node->text + " has no member called " + node->text, node->line);
            return nullptr;
        }
        if (can_use_member(access) == false) {
            add_error(node->text + " is private so only its own class can use it", node->line);
            return nullptr;
        }
        if (access->left->kind == NodeKind::Function) {
            add_error(node->text + " is a function so it needs ( ) to call it", node->line);
            return nullptr;
        }
        return declared_type(access->left);
    }

    // look through the structs fields for this name
    for (size_t i = 0; i < struct_node->children.size(); i = i + 1) {
        Node* field = struct_node->children[i];
        if (field->text == node->text) {
            return declared_type(field);
        }
    }
    add_error(struct_node->text + " has no field called " + node->text, node->line);
    return nullptr;
}

// can this type be turned into another one with as
bool can_convert(Node* type) {
    // numbers and pointers and true or false
    if (is_number_type(type) == true || type->text == "^" || type->text == "bool") {
        return true;
    }
    // an enum is a number underneath
    Node* found = find_name(type->text);
    return found != nullptr && found->kind == NodeKind::Enum;
}

// works out the type of a conversion like hits as float which is the type written after as
Node* type_of_cast(Node* node) {
    // if the type after as isnt real thats the only mistake to report
    size_t mistakes_before = checker_errors.size();
    check_type(node->right);
    if (checker_errors.size() > mistakes_before) {
        return nullptr;
    }
    Node* from = type_of(node->left);
    // nullptr means the value is unknown and its mistake is already recorded
    if (from != nullptr && (can_convert(from) == false || can_convert(node->right) == false)) {
        add_error("can't turn " + type_name(from) + " into " + type_name(node->right), node->line);
    }
    return node->right;
}

// works out the type of one item like scores[3] or p[3] or name[1]
Node* type_of_index(Node* node) {
    Node* base = type_of(node->left);
    Node* item_type = type_of(node->right);
    // nullptr means that part is unknown and its mistake is already recorded
    if (base == nullptr) {
        return nullptr;
    }

    // the item number has to be a whole number
    if (item_type != nullptr && is_whole_number_type(item_type) == false) {
        add_error("the item number has to be a whole number but this is " + type_name(item_type), node->line);
    }
    if (node->right->kind == NodeKind::Unary && node->right->text == "-") {
        add_error("the item number can't be negative", node->line);
    }

    // an array gives one of its items
    if (base->text == "[]") {
        // when the size and the item number are both typed right there they can be compared
        Node* size = base->right;
        Node* item = node->right;
        bool both_plain = size->kind == NodeKind::Number && item->kind == NodeKind::Number &&
                          size->text.find_first_not_of("0123456789") == string::npos &&
                          item->text.find_first_not_of("0123456789") == string::npos &&
                          size->text.size() < 10 && item->text.size() < 10;
        if (both_plain == true && stoi(item->text) >= stoi(size->text)) {
            add_error(item->text + " is past the end of the array (0 to " + to_string(stoi(size->text) - 1) + ")", node->line);
        }
        return base->left;
    }

    // a pointer can be used like an array that starts where it points
    if (base->text == "^") {
        return base->left;
    }

    // one letter of a string
    if (same_type(base, str_type) == true) {
        return char_type;
    }

    add_error(type_name(base) + " isn't an array so it has no items", node->line);
    return nullptr;
}

// works out the type of p^ which is whatever p points at
Node* type_of_follow(Node* node) {
    Node* pointer_type = type_of(node->left);
    // nullptr means its unknown and its mistake is already recorded
    if (pointer_type == nullptr) {
        return nullptr;
    }
    if (pointer_type->text != "^") {
        add_error("^ needs a pointer but this is " + type_name(pointer_type), node->line);
        return nullptr;
    }
    return pointer_type->left;
}

// promise bc type_of_call uses it and its written further down
void check_range(Node* target, Node* value);

// checks a calls inputs against the functions first line and gives back what the function promised
Node* check_call_inputs(Node* function, Node* call, string name, vector<Node*> &input_types) {
    // the right number of inputs
    Node* wanted = function->right;
    if (wanted->children.size() != call->children.size()) {
        string word = " inputs";
        if (wanted->children.size() == 1) {
            word = " input";
        }
        add_error(name + " needs " + to_string(wanted->children.size()) + word + " but got " + to_string(call->children.size()), call->line);
        return function->left;
    }

    // each input has to fit the spot the function gave it
    for (size_t i = 0; i < call->children.size(); i = i + 1) {
        Node* spot_type = wanted->children[i]->left;
        if (input_types[i] == nullptr) {
            continue;
        }
        if (type_fits(spot_type, input_types[i]) == false) {
            add_error("input " + to_string(i + 1) + " of " + name + ": can't put " + type_name(input_types[i]) + " into " + type_name(spot_type), call->line);
        } else {
            check_range(spot_type, call->children[i]);
        }
    }

    // the calls type is what the function promised to give back
    return function->left;
}

// works out the type of a call like add(1, 2) or enemy.hit(3)
Node* type_of_call(Node* node) {
    // every input gets its own type worked out first which also catches mistakes inside the inputs
    vector<Node*> input_types;
    for (size_t i = 0; i < node->children.size(); i = i + 1) {
        input_types.push_back(type_of(node->children[i]));
    }

    Node* called = node->left;

    // enemy.hit(3) calls a function that belongs to a class
    if (called->kind == NodeKind::Field) {
        Node* owner_type = type_of(called->left);
        if (owner_type == nullptr) {
            return nullptr;
        }
        // the dot works through a pointer too
        if (owner_type->text == "^") {
            owner_type = owner_type->left;
        }
        Node* class_node = find_name(owner_type->text);
        if (owner_type->text == "^" || class_node == nullptr || class_node->kind != NodeKind::Class) {
            add_error(type_name(owner_type) + " has no functions", node->line);
            return nullptr;
        }
        Node* access = find_member(class_node, called->text);
        if (access == nullptr) {
            add_error(class_node->text + " has no function called " + called->text, node->line);
            return nullptr;
        }
        if (can_use_member(access) == false) {
            add_error(called->text + " is private so only its own class can use it", node->line);
            return nullptr;
        }
        if (access->left->kind != NodeKind::Function) {
            add_error(called->text + " is not a function", node->line);
            return nullptr;
        }
        return check_call_inputs(access->left, node, called->text, input_types);
    }

    // add(1, 2) calls a function by its name
    if (called->kind != NodeKind::Name) {
        add_error("only a function can be called", node->line);
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
    return check_call_inputs(function, node, called->text, input_types);
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

    // math only works on numbers so not on bool or str or structs or enums
    if (is_number_type(shared) == false) {
        add_error("can't do math with " + type_name(shared), node->line);
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
        // a struct or enum or class name is a type not a value
        if (found->kind == NodeKind::Struct || found->kind == NodeKind::Enum || found->kind == NodeKind::Class) {
            add_error(node->text + " is a type not a value", node->line);
            return nullptr;
        }
        // a functions name by itself isnt a value
        if (found->kind == NodeKind::Function) {
            add_error(node->text + " is a function so it needs ( ) to call it", node->line);
            return nullptr;
        }
        // the type comes from the node that declared it
        return declared_type(found);
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

    if (node->kind == NodeKind::Field) {
        return type_of_field(node);
    }

    if (node->kind == NodeKind::Choice) {
        return type_of_choice(node);
    }

    if (node->kind == NodeKind::Index) {
        return type_of_index(node);
    }

    if (node->kind == NodeKind::Follow) {
        return type_of_follow(node);
    }

    if (node->kind == NodeKind::Cast) {
        return type_of_cast(node);
    }

    // this is a pointer to the object the function was called on
    if (node->kind == NodeKind::This) {
        if (current_class == nullptr) {
            add_error("this only works inside a class's function", node->line);
            return nullptr;
        }
        Node* pointer = make_node(NodeKind::Type, "^", node->line);
        pointer->left = make_node(NodeKind::Type, current_class->text, node->line);
        return pointer;
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
    // the type written has to be a real typ
    check_type(node->left);

    // an arrays sizes have to be whole numbers
    for (size_t i = 0; i < node->children.size(); i = i + 1) {
        Node* amount = node->children[i]->left;
        Node* amount_type = type_of(amount);
        if (amount_type != nullptr && is_whole_number_type(amount_type) == false) {
            add_error("an array's size has to be a whole number but this is " + type_name(amount_type), node->line);
        }
        if (amount->kind == NodeKind::Number && amount->text == "0") {
            add_error("an array's size can't be 0", node->line);
        }
    }

    // a const can never be changed so it has to start with a value
    if (has_const(node->left) == true && node->right == nullptr) {
        add_error(node->text + " is const so it needs a starting value", node->line);
    }

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

    // find the variable being stored into by walking down through fields and items like hero.items[2]
    Node* target = node->left;
    while (target->kind == NodeKind::Field || target->kind == NodeKind::Index) {
        target = target->left;
    }
    if (target->kind == NodeKind::Name) {
        Node* declared = find_name(target->text);
        if (declared != nullptr && declared->kind == NodeKind::Declare && has_const(declared->left) == true) {
            add_error(target->text + " is const so it can't be changed", node->line);
            return;
        }
    }

    // a whole array cant be stored into in one go
    if (left_type != nullptr && left_type->text == "[]") {
        add_error("can't store into a whole array so pick an item like [0]", node->line);
        return;
    }
    // nullptr means the type is unknown and its mistake is already recorded
    if (left_type != nullptr && right_type != nullptr && type_fits(left_type, right_type) == false) {
        add_error("can't put " + type_name(right_type) + " into " + type_name(left_type), node->line);
    } else if (left_type != nullptr && right_type != nullptr) {
        // the type is right so see if the number itself is small enough
        check_range(left_type, node->right);
    }
}

// checks a struct which is a list of fields
void check_struct(Node* node) {
    for (size_t i = 0; i < node->children.size(); i = i + 1) {
        Node* field = node->children[i];
        check_type(field->left);

        // a struct holding a whole copy of itself would never end
        if (field->left->text == node->text) {
            add_error("a struct can't hold itself so use " + node->text + "^", field->line);
        }

        // two fields cant share a name
        for (size_t j = 0; j < i; j = j + 1) {
            if (node->children[j]->text == field->text) {
                add_error(node->text + " already has a field called " + field->text, field->line);
            }
        }

        // a fields starting value has to fit it
        if (field->right != nullptr) {
            Node* value_type = type_of(field->right);
            if (value_type != nullptr && type_fits(field->left, value_type) == false) {
                add_error("can't put " + type_name(value_type) + " into " + type_name(field->left), field->line);
            } else if (value_type != nullptr) {
                check_range(field->left, field->right);
            }
        }
    }
}

// checks an enum which is a list of choices
void check_enum(Node* node) {
    for (size_t i = 0; i < node->children.size(); i = i + 1) {
        // two choices cant share a name
        for (size_t j = 0; j < i; j = j + 1) {
            if (node->children[j]->text == node->children[i]->text) {
                add_error(node->text + " already has a choice called " + node->children[i]->text, node->children[i]->line);
            }
        }
    }
}

// promise bc check_class checks the functions inside it
void check_function(Node* node);

// checks a class which is a list of fields and functions that can have a parent
void check_class(Node* node) {
    // the parent has to be another class
    if (node->left != nullptr) {
        Node* parent = find_name(node->left->text);
        if (parent == nullptr || parent->kind != NodeKind::Class) {
            add_error(node->left->text + " is not a class so it can't be a parent", node->line);
        } else if (parent == node) {
            add_error("a class can't be its own parent", node->line);
        }
    }

    current_class = node;
    open_level();

    // this classs own members become names its functions can use directly like hp instead of this.hp
    for (size_t i = 0; i < node->children.size(); i = i + 1) {
        Node* member = node->children[i]->left;
        Node* problem = declare_name(member->text, member);
        if (problem != nullptr) {
            checker_errors.push_back(problem);
        }
    }

    // then the parents members that arent private and a name this class already has wins
    Node* from = node;
    for (int steps = 0; steps < 50; steps = steps + 1) {
        if (from->left == nullptr) {
            break;
        }
        from = find_name(from->left->text);
        if (from == nullptr || from->kind != NodeKind::Class) {
            break;
        }
        for (size_t i = 0; i < from->children.size(); i = i + 1) {
            Node* access = from->children[i];
            if (access->text != "private") {
                // the mistake it gives back for a name thats already here is dropped on purpose
                declare_name(access->left->text, access->left);
            }
        }
    }

    // now check each member
    for (size_t i = 0; i < node->children.size(); i = i + 1) {
        Node* member = node->children[i]->left;
        if (member->kind == NodeKind::Function) {
            check_function(member);
        } else {
            check_type(member->left);
            // a class holding a whole copy of itself would never end
            if (member->left->text == node->text) {
                add_error("a class can't hold itself so use " + node->text + "^", member->line);
            }
            // a fields starting value has to fit it
            if (member->right != nullptr) {
                Node* value_type = type_of(member->right);
                if (value_type != nullptr && type_fits(member->left, value_type) == false) {
                    add_error("can't put " + type_name(value_type) + " into " + type_name(member->left), member->line);
                } else if (value_type != nullptr) {
                    check_range(member->left, member->right);
                }
            }
        }
    }

    close_level();
    current_class = nullptr;
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
    loops_deep = loops_deep + 1;
    for (size_t i = 0; i < node->children.size(); i = i + 1) {
        check_statement(node->children[i]);
    }
    loops_deep = loops_deep - 1;
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

    loops_deep = loops_deep + 1;
    for (size_t i = 0; i < node->children.size(); i = i + 1) {
        check_statement(node->children[i]);
    }
    loops_deep = loops_deep - 1;
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
    // a function goes at the top of the file or directly inside a class
    if (current_function != nullptr || (checker_level != 0 && current_class == nullptr)) {
        add_error("a function can't be inside another function or block", node->line);
    }

    // the return lines inside need to know which function theyre in
    current_function = node;
    open_level();

    // the type it gives back has to be a real type
    check_type(node->left);

    // the inputs are names the lines inside can use
    Node* inputs = node->right;
    for (size_t i = 0; i < inputs->children.size(); i = i + 1) {
        Node* input = inputs->children[i];
        check_type(input->left);
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
        case NodeKind::Struct:
            check_struct(node);
            break;
        case NodeKind::Enum:
            check_enum(node);
            break;
        case NodeKind::Class:
            check_class(node);
            break;
        case NodeKind::Break:
        case NodeKind::Continue:
            // these only make sense inside a while or for
            if (loops_deep == 0) {
                add_error(node->text + " can only be inside a loop", node->line);
            }
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
    current_class = nullptr;
    loops_deep = 0;

    // first pass: every function and struct name goes in the list so they can be used from anywhere in the file
    for (size_t i = 0; i < program->children.size(); i = i + 1) {
        Node* line = program->children[i];
        if (line->kind == NodeKind::Function || line->kind == NodeKind::Struct || line->kind == NodeKind::Enum || line->kind == NodeKind::Class) {
            Node* problem = declare_name(line->text, line);
            if (problem != nullptr) {
                checker_errors.push_back(problem);
            }
        }
    }

    // second pass: the real check
    for (size_t i = 0; i < program->children.size(); i = i + 1) {
        Node* line = program->children[i];
        // outside every function only declarations are allowed
        NodeKind k = line->kind;
        bool is_declaration = k == NodeKind::Declare || k == NodeKind::Function || k == NodeKind::Struct ||
                              k == NodeKind::Enum || k == NodeKind::Class || k == NodeKind::Blank;
        if (is_declaration == false) {
            add_error("this line has to go inside a function because only declarations can be outside", line->line);
            continue;
        }
        check_statement(line);
    }
    // the first pass can find a mistake on a later line before the second pass finds an earlier one
    stable_sort(checker_errors.begin(), checker_errors.end(), earlier_line);
    errors = checker_errors;
}
