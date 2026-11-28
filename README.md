# Console Language

This is my own programming language and its compiler, langc. The language doesn't have a name yet. I'm making it for a game console I'm building. The
compiler is written in C++ and turns programs into RISC-V assembly.

The language puts types first like C++, doesn't use semicolons, and closes every block with its own end word.

```
fn int add(int a, int b)
    int total = a + b
    return total
endfn
```

The lexer, the parser and the checker are finished. The parser reads a whole file into a tree: functions, structs, enums and classes (with `private` and a parent), if / elif / else, while and for blocks, declarations (`const int MAX = 100`, `Player^ p`, `char grid[8][8]`), calls, fields and indexes (`players[2].hp`), enum choices (`State::Walk`), lists, strings, casts (`count as float`) and math with the right order of operations. The checker then looks for mistakes in meaning: names that were never declared or are used outside their block, values of the wrong type, mixed types in math, numbers too big for their type (`char c = 500`), wrong inputs in a call, functions that don't return what they promised, fields that don't exist, storing into a `const`, and using a `private` member from outside its class. Every mistake is printed with its line number and which step found it. `build/langc.exe --tokens --ast FILE` prints the tokens and the tree. Next: the code generator, which writes RISC-V assembly.

To build it you need g++ and make. Run `make` in this folder and it builds `build/langc.exe`.
