SRC_DIR = src

CC = gcc

CFLAGS = -W -Wall -pedantic -ansi -lssl -lcrypto -lm
INCLUDES = -Iinclude

.PHONY: clean depend

all:
	$(CC) -o bin/steg $(INCLUDES) $(SRC_DIR)/*.c $(CFLAGS)

clean:
	@echo Cleaning...
	@rm -rf bin/steg *~
	@echo Done!