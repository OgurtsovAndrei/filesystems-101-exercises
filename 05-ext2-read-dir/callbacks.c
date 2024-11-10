#include "solution.h"

#include <stdio.h>

void report_file(int inode_nr, char type, const char *name)
{
	printf("%04i: %c %s\n", inode_nr, type, name);
}
