#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


//method for creating superblock CSV
void superblock(char* file);

void group(char* file);

int main(int argc, char* argv[])
{
	if (argc < 1)//no file
	{
		fprintf(stderr, "disk image required\n");
		exit(1);
	}
	//get name of image
	size_t len = strlen(argv[1]);
	char* imageName = malloc(sizeof(char)*len);
	strcpy(imageName, argv[1]); 

	/*------SUPER BLOCK--------*/
	superblock(imageName);
	/*------GROUP DESCRIPTORS--*/
	group(imageName);

}

void superblock(char* file)
{
	FILE* fil = fopen(file,"r");
	int fd = fileno(fil);
	FILE* csv = fopen("super.csv", "w+"); //truncate or create file
	/* there has to be a better way to do this, but copying characters 
	into separate buffers; and conduct sanity checks as we go */
	unsigned short magic;
	pread(fd, &magic, 2, 1024+56);
	if(magic != 0xEF53)
	{
	  fprintf(stderr, "Superblock - invalid magic: 0xdead/n");
	  exit(1);
	}
	unsigned int inodes;
	pread(fd, &inodes, 4, 1024);
	unsigned int blocks;
	pread(fd, &blocks, 4, 1024+4);
	unsigned int temp;
	pread(fd, &temp, 4, 1024+24);
	unsigned int bsize = 1024 << temp;
	if (bsize < 512 || bsize > 64000 )
	  {
	    fprintf(stderr, "Superblock - invalid block size: 666/n");
	    exit(1);
	  }
	pread(fd, &temp, 4, 1024+28);
	unsigned int fsize;
	//shift left if positive, right if ngative?
	fsize = 1024 << temp;
	unsigned int  bpg;
	pread(fd, &bpg, 4, 1024+32);
	if (blocks % bpg != 0)
	  {
	    fprintf(stderr, "Superblock - %d blocks, % blocks/group", blocks, bpg);
	    exit(1);
	  }
	unsigned int ipg;
	pread(fd, &ipg, 4, 1024+40);
	if (inodes % ipg != 0)
	  {
	    fprintf(stderr, "Superblock - %d Inodes, %d Inodes/group\n",inodes, ipg);
	    exit(1);
	  }
	unsigned int fpg;
	pread(fd, &fpg, 4, 1024+36);
	unsigned int fdb;
	pread(fd, &fdb, 4, 1024+20);
	fprintf(csv,"%02X,%d,%d,%d,%d,%d,%d,%d,%d\n", magic, inodes, blocks, bsize, fsize, bpg, ipg, fpg, fdb); 
}

void group(char* file)
{
  	FILE* fil = fopen(file,"r");
	int fd = fileno(fil);
	FILE* csv = fopen("group.csv", "w+"); //truncate or create file
	//# block groups = blocks / bpg
	unsigned short freeBs,freeIs, directories;
	unsigned int blocks, bpg, blockSize, iBit, bBit, inodeTable;
	pread(fd, &blocks, 4,1024+4);
	pread(fd, &bpg, 4, 1024+32);
	pread(fd, &blockSize, 4, 1024+24);
	blockSize = 1024 << blockSize;
	unsigned int groups = blocks / bpg;
	int i;
	for ( i = 0; i < groups; i++)
	  {
	    pread(fd, &freeBs, 2, 1024 + blockSize + 32*i + 12);
	    pread(fd, &freeIs, 2, 1024 + blockSize + 32*i + 14);
	    pread(fd, &directories, 2, 1024 + blockSize + 32*i + 16);
	    pread(fd, &iBit, 4, 1024 + blockSize + 32*i + 4);
	    pread(fd, &bBit, 4, 1024 + blockSize + 32*i);
	    pread(fd, &inodeTable, 4, 1024 + blockSize + 32*i + 8);
	    fprintf(csv, "%d, %d, %d, %d, %04X, %04X, %04X\n", bpg, freeBs, freeIs, directories, iBit, bBit, inodeTable);
	  }/*TODO PUT IN CHECKS*/
}
