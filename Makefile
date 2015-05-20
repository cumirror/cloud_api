test: client.c
	gcc -o test client.c crc32file.c encode.c

clean:
	rm test
