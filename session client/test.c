#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#define O_SESSION 00000010

int main() {
	int fd, written, r, ret;
	int status = 0;
	char buf_write[50] = { 0 };
	char buf_read[250] = { 0 };
	
	fd = open("test.txt",  O_CREAT|O_RDWR|O_TRUNC|O_SESSION, 0666);
	if (fd < 0) {
		printf("Error opening file \n");
		return -1;
	}
	
	sprintf(buf_write, "Writing file the first time... ");
	printf("First write\n\n");
	written = write(fd, buf_write, strlen(buf_write));
	if (written < 0) {
		printf("Error writing file \n");
		return -1;
	}
	
	sleep(2);
	
	ret = fork();
	if (ret < 0) {
		printf("Error forking process \n");
		return -1;
	} else if (ret == 0) { //child
		lseek(fd, 0, SEEK_END);
		sprintf(buf_write, "I'm the child and I'm writing... ");
		printf("[child] I'm writing\n");
		written = write(fd, buf_write, strlen(buf_write));
		if (written < 0) {
			printf("[child] Error writing file \n");
			return -1;
		}
		
		ret = close(fd);
		if (ret < 0) {
			printf("[child] Error closing file \n");
			return -1;
		}
		printf("[child] I've closed the file, but the session still exists because it was shared \n");

		return 0;
	}
	
	//father
	wait(&status);
	
	lseek(fd, 0, SEEK_SET);
	int count = 0;
	do {
		r = read(fd, buf_read + count, sizeof(char));
		count += r;
		
		if (r < 0 || count >= 250)
			break;
	} while (r > 0);

	if (read < 0) {
		printf("[father] Error reading file \n");
		return -1;
	}
	
	printf("[father] I read: %s\n", buf_read);
	
	printf("* Sleeping 20 seconds, check that I read from the buffer and not from the file\n\n");
	sleep(20);
	
	lseek(fd, 0, SEEK_END);
	sprintf(buf_write, "I'm the father and I'm writing... ");
	printf("[father] I'm writing \n");
	written = write(fd, buf_write, strlen(buf_write));
	if (written < 0) {
		printf("[father] Error writing file \n");
		return -1;
	}
	
	ret = close(fd);
	if (ret < 0) {
		printf("[father] Error closing file, now the session is removed \n");
		return -1;
	}
	
	printf("[father] I've closed the file \n");
	
	return 0;
}
