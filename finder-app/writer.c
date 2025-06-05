#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>

const int expected_args = 3;

int main(int argc, char *argv[]) {
	openlog("writer", 0, LOG_USER);
	
	printf("num args: %d\n", argc);
	if (argc != expected_args)
	{
		printf("Invalid number of arguments: %d, expected %d\n", argc, expected_args);
		syslog(LOG_ERR, "Invalid number of arguments: %d, expected 3\ns", expected_args);
		return 1;
	}

	const char *filename = argv[1];
	const char *inputString = argv[2];

	FILE *file = fopen(filename, "w");
	syslog(LOG_DEBUG, "Writing %s to %s\n", inputString, filename);
	fprintf(file, "%s", inputString);
	fclose(file);

	for (int i = 0; i < argc; i++)
	{
		printf("Arg %d: %s\n", i, argv[i]);
	}

	return 0;
}
