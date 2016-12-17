/* 
COMP3005 Operating Systems Coursework 2
- Jasmine Lu 14002484 
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "bootsect.h"
#include "bpb.h"
#include "direntry.h"
#include "fat.h"
#include "dos.h"

void usage() {
    fprintf(stderr, "Usage: dos_scandisk <imagename>\n");
    exit(1);
}

int main(int argc, char** argv) {
	
	uint8_t *image_buf;
	int fd;
	struct bpb33* bpb;
	if (argc != 2) {
		usage();
	}

	image_buf = mmap_file(argv[1], &fd);
    bpb = check_bootsector(image_buf);

    int number_of_clusters = bpb->bpbSectors / bpb->bpbSecPerClust;
    int size_of_cluster = bpb->bpbSecPerClust * bpb->bpbBytesPerSec;

    int referenced_clusters[number_of_clusters];

}