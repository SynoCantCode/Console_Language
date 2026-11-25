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

The lexer is finished, and the parser can now read a whole file of declarations (`const int MAX = 100`, `Player^ p`, `char grid[8][8]`), assignments and math, with the right order of operations, brackets, negatives and `not`. Mistakes get printed with their line number. `build/langc.exe --tokens --ast FILE` prints the tokens and the tree it builds.

To build it you need g++ and make. Run `make` in this folder and it builds `build/langc.exe`.
