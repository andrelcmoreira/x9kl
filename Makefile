CC = g++
BIN = x9kl
LOGS_DIR ?= $(PWD)/.$(BIN)
CFLAGS = -s -O1 -D LOGS_DIR=\"$(LOGS_DIR)\"
CFLAGS_DBG = -g -D DEBUG=ON -D LOGS_DIR=\"$(LOGS_DIR)\"

.PHONY: clean x9kl x9kl-dbg

$(BIN): $(BIN).cc
	$(CC) $(CFLAGS) $< -o $(BIN)

$(BIN)-dbg: $(BIN).cc
	$(CC) $(CFLAGS_DBG) $< -o $(BIN)

clean:
	@rm -rf $(LOGS_DIR)
	@rm -f $(OUT)
