#include "fs_malloc.h"

#include <stdlib.h>
#include <string.h>
#include <err.h>

void* fs_xmalloc(size_t size)
{
	void *x = malloc(size);
	if (x == NULL)
		errx(1, "malloc() failed");
	return x;
}

void* fs_xzalloc(size_t size)
{
	void *x = fs_xmalloc(size);
	memset(x, 0, size);
	return x;
}

void* fs_xrealloc(void *x, size_t size)
{
	x = realloc(x, size);
	if (x == NULL)
		errx(1, "realloc() failed");
	return x;
}

void fs_xfree(void *x)
{
	free(x);
}
