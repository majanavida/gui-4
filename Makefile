CXX = g++
CXXFLAGS = -std=c++17

scrum_board.exe: src/main.cpp src/ScrumBoard.cpp src/ScrumBoard.h
	$(CXX) $(CXXFLAGS) src/main.cpp src/ScrumBoard.cpp -o $@

tests.exe: tests/tests.cpp src/ScrumBoard.cpp src/ScrumBoard.h
	$(CXX) $(CXXFLAGS) -Isrc tests/tests.cpp src/ScrumBoard.cpp -o $@ -lgtest_main -lgtest

.PHONY: test
test: tests.exe
	./tests.exe
