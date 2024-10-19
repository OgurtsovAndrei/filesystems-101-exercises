#include <solution.h>

#include <stdio.h>

int helloworld(const char *mntp);

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "use: %s <mount-point>\n", argv[0]);
		return 1;
	}

	helloworld(argv[1]);
	return 0;
}
