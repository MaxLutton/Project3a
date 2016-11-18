#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


//method for creating superblock CSV
void superblock(char* file);

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
		errno = 0xdead;
		perror("invalid magic");
		exit(1);
	}
	unsigned int inodes;
	pread(fd, &inodes, 4, 1024);
	unsigned int blocks;
	pread(fd, &blocks, 4, 1024+4);
	unsigned int temp;
	pread(fd, &temp, 4, 1024+24);
	unsigned int bsize = 1024 << temp;
	pread(fd, &temp, 4, 1024+28);
	unsigned int fsize;
	//shift left if positive, right if ngative?
	fsize = 1024 << temp;
	unsigned int  bpg;
	pread(fd, &bpg, 4, 1024+32);
	unsigned int ipg;
	pread(fd, &ipg, 4, 1024+40);
	unsigned int fpg;
	pread(fd, &fpg, 4, 1024+36);
	unsigned int fdb;
	pread(fd, &fdb, 4, 1024+20);
	fprintf(csv,"%02X,%d,%d,%d,%d,%d,%d,%d,%d\n", magic, inodes, blocks, bsize, fsize, bpg, ipg, fpg, fdb); 
}
