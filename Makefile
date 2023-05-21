CC = g++
CFLAGS = -O1
OUT = x9kl

.PHONY: daemon clean debug

daemon:
	$(CC) $(CFLAGS) $(OUT).cc -o $(OUT)

debug:
	$(CC) -D DEBUG=ON $(CFLAGS) $(OUT).cc -o $(OUT)

clean:
	@rm -f $(OUT)
