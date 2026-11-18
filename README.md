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

Right now the lexer is finished, and I'm working on the parser next.

To build it you need g++ and make. Run `make` in this folder and it builds `build/langc.exe`.
