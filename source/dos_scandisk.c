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

struct {
	int number_of_clusters;
	int bytes_per_cluster;
} cluster_info;

void print_unreferenced_clusters(int referenced_clusters[], uint8_t *image_buf, struct bpb33* bpb) {
	
	printf("Unreferenced: ");
	int i = 2;
	for(i; i < cluster_info.number_of_clusters; i++) {
		int next_cluster = get_fat_entry(i, image_buf, bpb);
		if ((next_cluster != 0) && (referenced_clusters[i] != -1)) {
			printf("%i ", i);
		}
	}
	printf("\n");
}

void mark_cluster_referenced(int referenced_clusters[], uint16_t cluster, uint8_t *image_buf, struct bpb33* bpb) {
	

	referenced_clusters[cluster] = -1;

	// printf("cluster %i: %i referenced: %i\n", cluster, get_fat_entry(cluster, image_buf, bpb) ,referenced_clusters[cluster]);

	uint16_t next_cluster = get_fat_entry(cluster, image_buf, bpb);
	if (is_end_of_file(next_cluster)) {
		return;
	} else {
		mark_cluster_referenced(referenced_clusters, next_cluster, image_buf, bpb);
	}
}

void follow_dir(int referenced_clusters[], uint16_t cluster, uint8_t *image_buf, struct bpb33* bpb) {
    struct direntry *dirent;
    int d, i;
    dirent = (struct direntry*)cluster_to_addr(cluster, image_buf, bpb);
    while (1) {
		for (d = 0; d < cluster_info.number_of_clusters; d += sizeof(struct direntry)) {

		    char name[9];
		    char extension[4];
		    uint16_t file_cluster;
		    name[8] = ' ';
		    extension[3] = ' ';
		    memcpy(name, &(dirent->deName[0]), 8);
		    memcpy(extension, dirent->deExtension, 3);
		    
		    if (name[0] == SLOT_EMPTY)
				return;

		    /* skip over deleted entries */
		    if (((uint8_t)name[0]) == SLOT_DELETED)
				continue;

		    /* names are space padded - remove the spaces */
		    for (i = 8; i > 0; i--) {
				if (name[i] == ' ') 
				    name[i] = '\0';
				else 
				    break;
		    }

		    /* remove the spaces from extensions */
		    for (i = 3; i > 0; i--) {
				if (extension[i] == ' ') 
				    extension[i] = '\0';
				else 
				    break;
		    }

		    /* don't print "." or ".." directories */
		    if (strcmp(name, ".")==0) {
				dirent++;
				continue;
		    }
		    if (strcmp(name, "..")==0) {
				dirent++;
				continue;
		    }

		    if ((dirent->deAttributes & ATTR_VOLUME) != 0) { 
		    } else if ((dirent->deAttributes & ATTR_DIRECTORY) != 0) {
				file_cluster = getushort(dirent->deStartCluster);
				follow_dir(referenced_clusters, file_cluster, image_buf, bpb);
				referenced_clusters[file_cluster] = -1;
		    } else {
				uint16_t file_start_cluster = getushort(dirent->deStartCluster);
				mark_cluster_referenced(referenced_clusters, file_start_cluster, image_buf, bpb);
		    }
		    dirent++;
		}
		
		if (cluster == 0) {
		    // root dir is special
		    dirent++;
		} else {
		    cluster = get_fat_entry(cluster, image_buf, bpb);
		    dirent = (struct direntry*)cluster_to_addr(cluster, image_buf, bpb);
		}

    }
}

void usage() {
    fprintf(stderr, "Usage: dos_scandisk <imagename>\n");
    exit(1);
}

int main(int argc, char** argv) {
	struct cluster_info;
	uint8_t *image_buf;
	int fd;
	struct bpb33* bpb;
	if (argc != 2) {
		usage();
	}

	image_buf = mmap_file(argv[1], &fd);
    bpb = check_bootsector(image_buf);

    cluster_info.number_of_clusters = bpb->bpbSectors / bpb->bpbSecPerClust;
    cluster_info.bytes_per_cluster = bpb->bpbSecPerClust * bpb->bpbBytesPerSec;

    int referenced_clusters[cluster_info.number_of_clusters];
    int j = 0;
    for(j; j < cluster_info.number_of_clusters; j++) {
    	referenced_clusters[j] = 0;
    }

    follow_dir(referenced_clusters, 0, image_buf, bpb);
    print_unreferenced_clusters(referenced_clusters, image_buf, bpb);

    // int i = 0;
    // for(i; i < cluster_info.number_of_clusters; i++) {
    // 	printf("cluster %i: %i referenced: %i\n", i, get_fat_entry(i, image_buf, bpb) ,referenced_clusters[i]);
    // }

}