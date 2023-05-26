CC = g++
OUT = x9kl
LOGS_DIR ?= $(PWD)/.$(OUT)/
CFLAGS = -O1 -D LOGS_DIR=\"$(LOGS_DIR)\"

.PHONY: daemon clean debug

daemon:
	$(CC) $(CFLAGS) $(OUT).cc -o $(OUT)

debug:
	$(CC) -D DEBUG=ON $(CFLAGS) $(OUT).cc -o $(OUT)

clean:
	@rm -f $(OUT)
