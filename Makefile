#Builds compiler
#make
#make clean

build/langc.exe: src/main.cpp src/token.cpp src/lexer.cpp src/token.hpp src/lexer.hpp
	mkdir -p build
	g++ -std=c++20 -Wall -Wextra -g src/main.cpp src/token.cpp src/lexer.cpp -o build/langc.exe

clean:
	rm -rf build

hello:
	echo hello!