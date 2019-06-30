
CXX = g++ -std=c++17 
#CXX    = clang++ -std=c++17
CFLAGS += -O3 -g3 -ggdb
IFLAGS += -Wall -Wextra

# where you've cloned find.hpp to
findhpp_dir = ../find
optionparser_dir = ../optionparser
IFLAGS += -I$(findhpp_dir) -I$(optionparser_dir)

## uncomment on cygwin!
LFLAGS += -lstdc++fs
## uncomment if you don't run cygwin, and/or don't have msvc, etc
#LFLAGS += -lboost_system -lboost_filesystem

# uncomment if you want/need debug info
#CFLAGS += -g3 -ggdb
#LFLAGS += -lmsvcrtd

src = main.cpp
exe = countext.exe

all:
	$(CXX) $(CFLAGS) $(IFLAGS) $(src) -o $(exe) $(LFLAGS)

