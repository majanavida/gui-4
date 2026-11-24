CXX = g++
CXXFLAGS = -std=c++17

SOURCES = src/main.cpp src/ConsoleMenu.cpp src/BoardFiles.cpp src/ScrumBoard.cpp
HEADERS = src/ConsoleMenu.h src/BoardFiles.h src/ScrumBoard.h

scrum_board.exe: $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $@

tests.exe: tests/tests.cpp src/ScrumBoard.cpp src/ScrumBoard.h
	$(CXX) $(CXXFLAGS) -Isrc tests/tests.cpp src/ScrumBoard.cpp -o $@ -lgtest_main -lgtest

.PHONY: test
test: tests.exe
	./tests.exe
