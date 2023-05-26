CC = g++
BIN = x9kl
LOGS_DIR ?= $(PWD)/.$(BIN)/
CFLAGS = -O1 -D LOGS_DIR=\"$(LOGS_DIR)\"

.PHONY: clean daemon debug

daemon:
	$(CC) $(CFLAGS) $(BIN).cc -o $(BIN)

debug:
	$(CC) -D DEBUG=ON $(CFLAGS) $(BIN).cc -o $(BIN)

clean:
	@rm -rf $(LOGS_DIR)
	@rm -f $(OUT)
