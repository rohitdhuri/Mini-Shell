default: shell.c
	gcc -o shell shell.c

clean: shell
	-rm shell