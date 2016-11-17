#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char* argv[])
{
	if (argc < 2)//no file
	{
		fprintf(stderr, "disk image required\n");
		exit(1);
	}
	//get name of image
	size_t len = strlen(argv[1]);
	char* imageName = malloc(sizeof(char)*len);
	strcpy(imageName, argv[1]); 

	/*------SUPER BLOCK--------*/

}
