compile: ftserver.c
	gcc -o ftserver ftserver.c

clean:
	$(RM) ftserver
