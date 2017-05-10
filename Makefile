llg: llg.c
	$(CC) -O3 -march=native -mtune=native -o llg -g llg.c
clean:
	rm llg
