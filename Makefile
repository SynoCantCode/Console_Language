#Builds compiler
#make
#make clean

build/langc.exe: src/main.cpp src/token.cpp src/lexer.cpp src/ast.cpp src/parser.cpp src/token.hpp src/lexer.hpp src/ast.hpp src/parser.hpp
	mkdir -p build
	g++ -std=c++20 -Wall -Wextra -g src/main.cpp src/token.cpp src/lexer.cpp src/ast.cpp src/parser.cpp -o build/langc.exe

clean:
	rm -rf build
