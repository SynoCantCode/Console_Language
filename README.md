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

The lexer and the parser are finished. The parser reads a whole file into a tree: functions, structs, enums and classes (with `private` and a parent), if / elif / else, while and for blocks, declarations (`const int MAX = 100`, `Player^ p`, `char grid[8][8]`), calls, fields and indexes (`players[2].hp`), lists, strings, casts (`count as float`) and math with the right order of operations. Mistakes get printed with their line number. `build/langc.exe --tokens --ast FILE` prints the tokens and the tree it builds. The checker (mistakes in meaning) is in progress: it catches names that were never declared, values of the wrong type (`int x = 2.5`), mixed types in math, and numbers too big for their type (`char c = 500`).

To build it you need g++ and make. Run `make` in this folder and it builds `build/langc.exe`.
