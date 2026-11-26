#include "checker.hpp"
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
