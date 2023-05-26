CC = g++
BIN = x9kl
LOGS_DIR ?= $(PWD)/.$(BIN)/
CFLAGS_DAEMON = -s -O1 -D LOGS_DIR=\"$(LOGS_DIR)\"
CFLAGS_DEBUG = -g -D DEBUG=ON -D LOGS_DIR=\"$(LOGS_DIR)\"

.PHONY: clean daemon debug

daemon:
	$(CC) $(CFLAGS_DAEMON) $(BIN).cc -o $(BIN)

debug:
	$(CC) $(CFLAGS_DEBUG) $(BIN).cc -o $(BIN)

clean:
	@rm -rf $(LOGS_DIR)
	@rm -f $(OUT)
