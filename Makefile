SRC_DIR = src

CC = g++

CFLAGS = -W -Wall -pedantic -ansi -std=c++11
INCLUDES = -Iinclude

.PHONY: clean depend

all:
	$(CC) -o bin/steg $(INCLUDES) $(SRC_DIR)/*.cpp $(CFLAGS) -lpthread

clean:
	@echo Cleaning...
	@rm -rf bin/steg *~
	@echo Done!