#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <linux/types.h>

//method for creating superblock CSV
void superblock(int fd);

void group(int fd);

void bitmap(int fd);

//void inode(int fd);

void inode(int fd, int number, int group, int offset);
struct inode
{
 __u16 i_mode;
 __u16 i_uid;
 __u32 i_size;
 __u32 i_atime;
 __u32 i_ctime;
 __u32 i_mtime;
  __u32 i_dtime;
  __u16 i_gid;
  __u16 i_links_count;
  __u32 i_blocks;
  __u32 i_flags;
  __u32 i_osd1;
  __u32 i_block[15];
  __u32 i_generation;
  __u32 i_file_acl;
  __u32 i_dir_acl;
  __u32 i_faddr;
  __u8 i_osd2[12];
};

static FILE* inodeF;


int main(int argc, char* argv[])
{
	if (argc < 1)//no file
	{
		fprintf(stderr, "disk image required\n");
		exit(1);
	}
	//get name of image
	size_t len = strlen(argv[1]);
	char* imageName = malloc(sizeof(char)*len+1);
	strcpy(imageName, argv[1]);
	FILE* fil = fopen(imageName,"r");
	int fd = fileno(fil);

	inodeF = fopen("inode.csv", "w+"); //truncate or create file
      

	/*------SUPER BLOCK--------*/
	superblock(fd);
	/*------GROUP DESCRIPTORS--*/
	group(fd);
	/*------BITMAP-------------*/
	bitmap(fd);
	/*------INODE--------------*/
	//	inode(fd);

	fclose(fil);
	free(imageName);

}

void superblock(int fd)
{
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
	fprintf(csv,"%x,%d,%d,%d,%d,%d,%d,%d,%d\n", magic, inodes, blocks, bsize, fsize, bpg, ipg, fpg, fdb);

	fclose(csv);
}

void group(int fd)
{
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
	    fprintf(csv, "%d,%d,%d,%d,%x,%x,%x\n", bpg, freeBs, freeIs, directories, iBit, bBit, inodeTable);
	  }/*TODO PUT IN CHECKS*/

	fclose(csv);
}

void bitmap(int fd)
{
  FILE* csv = fopen("bitmap.csv", "w+"); //truncate or create file
  unsigned int bpg, ipg, blockSize, blocks;
  pread(fd, &bpg, 4, 1024+32);
  pread(fd, &ipg, 4, 1024 + 40);
  pread(fd, &blockSize, 4, 1024+24);
  pread(fd, &blocks, 4, 1024+4);
  blockSize = 1024 << blockSize;
  unsigned int groups = blocks/bpg;
  int bMap, iMap;
  unsigned int currentBlock, currentInode;
  char byte, mask;
  currentBlock = 0;
  currentInode = 0;
  int i, j;
  //iterate for each block group
  for (i = 0; i < groups; i++)
    {
      pread(fd, &bMap, 4, 1024 + blockSize + 32*i);//block bitmap block number by reading from group descriptors
      pread(fd, &iMap, 4, 1024 + blockSize + 32*i + 4);//inode bitmap block number
      
      //for each bit in this group's bitmaps
      for (j = 0; j < bpg; j++)
	{
	  int byte_offset = j / 8;
	  int bit_offset = j % 8;
	  mask = (1 << bit_offset);
	  pread(fd, &byte, 1, blockSize*bMap+ byte_offset);//read from block bitmap
	  if ( !(byte & mask) )
	    fprintf(csv, "%x,%d\n", bMap, currentBlock+1);
	  currentBlock++;
	}
      for (j = 0; j < ipg; j++)
	{
	 int byte_offset = j / 8;
	 int bit_offset = j % 8;
	 mask = (1 << bit_offset);
	 pread(fd, &byte, 1, blockSize*iMap + byte_offset);//read from inode bitmap
	 if ( !(byte & mask) )
	   fprintf(csv, "%x,%d\n", iMap, currentInode+1);
	 else
	   inode(fd, currentInode,i,j); //only print to csv when used inode
	 currentInode++;
	}
    }
  fclose(csv);
}

void inode(int fd, int number, int group, int offset)
{


  unsigned short inodeSize;
  unsigned int bpg, ipg, blockSize, blocks, groups, numInodes, freeInodes;

  pread(fd, &inodeSize, 2, 1024+88);
  inodeSize = (1024 << inodeSize) / 8;
  pread(fd, &bpg, 4, 1024+32);
  pread(fd, &ipg, 4, 1024+40);
  pread(fd, &blockSize, 4, 1024+24);
  pread(fd, &blocks, 4, 1024+4);
  blockSize = 1024 << blockSize;
  groups = blocks / bpg;
  pread(fd, &numInodes, 4, 1024);
  pread(fd, &freeInodes, 4, 1024+16);

  unsigned int tableBg;
  int i, j, k;
  unsigned int currentInode = 1;
  struct inode myNode;
  char fileType;
  pread(fd, &tableBg, 4, 1024+blockSize + 32*group + 8);
  pread(fd, &myNode, sizeof(myNode), blockSize*tableBg +inodeSize*offset);

  if (myNode.i_mode & 0x8000)
    fileType = 'f';
  else if (myNode.i_mode & 0x4000)
    fileType = 'd';
  else if (myNode.i_mode & 0xA000)
    fileType = 's';
  else
    fileType = '?';
  
  fprintf(inodeF, "%d,%c,%o,%d,%d,%d,%x,%x,%x,%d,%d,", number+1,
	 fileType, myNode.i_mode,myNode.i_uid, myNode.i_gid,
	 myNode.i_links_count,myNode.i_ctime, myNode.i_mtime,
	 myNode.i_atime, myNode.i_size, myNode.i_blocks);
  for (k = 0; k < 14; k++)
    fprintf(inodeF, "%x,", myNode.i_block[k]);
  fprintf(inodeF, "%x\n", myNode.i_block[14]);
 

}
/*
void inode(int fd)
{
  FILE* csv = fopen("inode.csv", "w+"); //truncate or create file

  unsigned short inodeSize;
  unsigned int bpg, ipg, blockSize, blocks, groups, numInodes, freeInodes;
  pread(fd, &inodeSize, 2, 1024+88);
  inodeSize = (1024 << inodeSize) / 8;
  pread(fd, &bpg, 4, 1024+32);
  pread(fd, &ipg, 4, 1024+40);
  pread(fd, &blockSize, 4, 1024+24);
  pread(fd, &blocks, 4, 1024+4);
  blockSize = 1024 << blockSize;
  groups = blocks / bpg;
  pread(fd, &numInodes, 4, 1024);
  pread(fd, &freeInodes, 4, 1024+16);
  int totalInodes = numInodes - freeInodes;
  /*inodes start at 1*/
  //for every block group
  /*unsigned int tableBg;
  int i, j, k;
  unsigned int currentInode = 1;
  struct inode myNode;
  char fileType;
  for (i = 0; i < groups; i++)
    {
      //block #  for inode table
      pread(fd, &tableBg, 4, 1024+blockSize + 32*i + 8);
      //for all inodes in group
      for (j = 0; j < ipg; j++)
	{
	  pread(fd, &myNode, sizeof(myNode), blockSize*tableBg + inodeSize*j);
	  if (myNode.i_mode & 0x8000)
	    fileType = 'f';
	  else if (myNode.i_mode & 0x4000)
	    fileType = 'd';
	  else if (myNode.i_mode & 0xA000)
	    fileType = 's';
	  else
	    fileType = '?';
  
	  fprintf(csv, "%d,%c,%o,%d,%d,%d,%x,%x,%x,%d,%d,", currentInode,
		 fileType, myNode.i_mode,myNode.i_uid, myNode.i_gid,
		 myNode.i_links_count,myNode.i_ctime, myNode.i_mtime,
		 myNode.i_atime, myNode.i_size, myNode.i_blocks);
	  for (k = 0; k < 14; k++)
	    fprintf(csv, "%x,", myNode.i_block[k]);
	  fprintf(csv, "%x\n", myNode.i_block[14]);
	  currentInode++;
	}
      if (currentInode > totalInodes)
	break;
    }

  fclose(csv);
  }
*/

	
  
