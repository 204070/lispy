#include <stdio.h>

// Declare a buffer for user input of size 2048
static char input[2048];

int main(int argc, char **argv)
{
	puts("Lispy Version 0.0.1");
	puts("Press Ctrl+c to exit\n");

	while (1)
	{
		fputs("lispy> ", stdout);

		// Read a line of user input of maximum size 2048
		fgets(input, 2048, stdin);

		printf("No you're a %s", input);
	}

	return 0;
}
