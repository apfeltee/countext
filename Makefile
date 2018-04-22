
CXX    = clang++ -std=c++17

CFLAGS += -Wall -Wextra -O3
IFLAGS +=
## uncomment on cygwin!
#LFLAGS += -lboost_system -lboost_filesystem

src = main.cpp
exe = countext.exe

all:
	$(CXX) $(CFLAGS) $(IFLAGS) $(src) -o $(exe) $(LFLAGS)

