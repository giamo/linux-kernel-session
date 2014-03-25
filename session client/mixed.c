#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define O_SESSION 00000010
#define NUM_THREADS 21

static pthread_t threads[NUM_THREADS];
static int fd;
static int id[NUM_THREADS];
char chars[21] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'l',
							'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'z'};

void write_on_file(int *arg) {
	int j, written, num;
	int size = 20;
	
	num = *((int*)arg);
	
	char buf_write[size];
	for (j = 0; j < size - 1; j++)
		buf_write[j] = chars[num];
	buf_write[size - 1] = 0;
	
	lseek(fd, 0, SEEK_END);
	written = write(fd, buf_write, strlen(buf_write));
	if (written < 0) {
		printf("[Thread %d] Error writing file \n", num);
		return;
	}
	
	printf("Written characters %c\n", chars[num]);
}

int main() {
	int i;
	bzero(threads, NUM_THREADS);
	
	fd = open("mixed.txt", O_CREAT|O_RDWR|O_TRUNC|O_SESSION, 0666);
	if (fd < 0) {
		printf("Error opening file \n");
		return -1;
	}
	
	for (i = 0; i < NUM_THREADS; ++i) {
		id[i] = i;
		pthread_create(&threads[i], NULL, (void*)write_on_file, &id[i]);
	}
	
	for(i = 0; i < NUM_THREADS; i++)
		pthread_join(threads[i], NULL);
	
	return 0;
}
