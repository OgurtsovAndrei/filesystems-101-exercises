#include "solution.h"

#include "fs_ext2.h"

int dump_file(int img, int inode_nr, int out)
{
	return dump_ext2_file(img, inode_nr, out);
}
