CXX = g++
CXXFLAGS = -std=c++17
FTXUI_VERSION = 7.0.3
FTXUI_DIR = build/ftxui-$(FTXUI_VERSION)
FTXUI_URL = https://github.com/ArthurSonzogni/FTXUI/releases/download/v$(FTXUI_VERSION)/ftxui-amalgamated.zip
FTXUI_FLAGS = -DUNICODE -D_UNICODE -DNOMINMAX
SOURCES = src/main.cpp src/BoardFiles.cpp src/ScrumBoard.cpp

.DELETE_ON_ERROR:

scrum_board.exe: $(SOURCES) src/BoardFiles.h src/ScrumBoard.h $(FTXUI_DIR)/ftxui.o Makefile
	$(CXX) $(CXXFLAGS) $(FTXUI_FLAGS) -I$(FTXUI_DIR) $(SOURCES) $(FTXUI_DIR)/ftxui.o -o $@ -pthread

$(FTXUI_DIR)/ftxui.o: $(FTXUI_DIR)/ftxui.cpp $(FTXUI_DIR)/ftxui.hpp Makefile
	$(CXX) $(CXXFLAGS) $(FTXUI_FLAGS) -c $< -o $@ -pthread

$(FTXUI_DIR)/ftxui.cpp $(FTXUI_DIR)/ftxui.hpp &: $(FTXUI_DIR)/ftxui.zip
	tar.exe -xmf $< -C $(FTXUI_DIR) ftxui.cpp ftxui.hpp

$(FTXUI_DIR)/ftxui.zip:
	curl.exe -fLsS --create-dirs -o $@ $(FTXUI_URL)

tests.exe: tests/tests.cpp src/ScrumBoard.cpp src/ScrumBoard.h
	$(CXX) $(CXXFLAGS) -Isrc tests/tests.cpp src/ScrumBoard.cpp -o $@ -lgtest_main -lgtest

.PHONY: test
test: tests.exe
	./tests.exe
