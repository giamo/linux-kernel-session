
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <linux/types.h>
#include <linux/fs.h>
#define O_SESSION 00000010

int main(int argc, char **argv) {
	int fd, wr;
	char in;

	if (argc < 2) {
		printf("./simple_write STRING_TO_WRITE\n");
		return -1;
	}
	
	fd = open("simple_write.txt", O_CREAT|O_RDWR|O_TRUNC|O_SESSION, 0666);
	lseek(fd, 0, SEEK_SET);

	wr = write(fd, argv[1], strlen(argv[1]));
	printf("Insert a character to continue: ");
	scanf("%c", &in);
	close(fd);
	return 0;
}
