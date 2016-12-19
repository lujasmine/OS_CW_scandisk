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

//part 2 - lists out the lost files
void find_lost_files(int cluster_tag[], uint16_t cluster, uint8_t *image_buf, struct bpb33* bpb) {
	int count = 0;
	int lost_file_start_cluster = -1;
	for(cluster; cluster <= cluster_info.number_of_clusters; cluster++) {
		int next_cluster = get_fat_entry(cluster, image_buf, bpb);
		if (cluster_tag[cluster] == -2) {
			count++;
			if (lost_file_start_cluster == -1) {
				lost_file_start_cluster = cluster;
			}
			if (is_end_of_file(next_cluster)) {
				printf("Lost File: %i %i\n", lost_file_start_cluster, count);
				// printf("next call starting on cluster %i\n", cluster+1);
				lost_file_start_cluster = -1;
				count = 0;
			} 
		}
	}
}

//part 1 - find all unreferenced clusters
void find_unreferenced_clusters(int cluster_tag[], uint8_t *image_buf, struct bpb33* bpb) {
	
	printf("Unreferenced: ");
	int i = 2;
	for(i; i < cluster_info.number_of_clusters; i++) {
		int next_cluster = get_fat_entry(i, image_buf, bpb);
		if ((next_cluster != 0) && (cluster_tag[i] != -1)) { //if cluster is neither free in the FAT nor used in any file
			printf("%i ", i);
			cluster_tag[i] = -2;//mark unreferenced cluster with -2 'tag'
		} 
	}
	printf("\n");

}

//mark cluster as referenced
void mark_referenced_cluster(int cluster_tag[], uint16_t cluster, uint16_t end_cluster, uint8_t *image_buf, struct bpb33* bpb) {

	cluster_tag[cluster] = -1; //mark referenced clusters with -1 'tag'

	// printf("cluster %i: %i referenced: %i\n", cluster, get_fat_entry(cluster, image_buf, bpb) ,cluster_tag[cluster]);

	uint16_t next_cluster = get_fat_entry(cluster, image_buf, bpb);
	if (is_end_of_file(next_cluster)) {
		return;
	} else {
		mark_referenced_cluster(cluster_tag, next_cluster, end_cluster, image_buf, bpb);
	}
}

//walks through directory tree
void follow_dir(int cluster_tag[], uint16_t cluster, uint8_t *image_buf, struct bpb33* bpb) {
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
				follow_dir(cluster_tag, file_cluster, image_buf, bpb);
				cluster_tag[file_cluster] = -1; //mark the clusters referenced by directories with -1 'tag'
		    } else {
				uint16_t file_start_cluster = getushort(dirent->deStartCluster);
				uint16_t file_size = getushort(dirent->deFileSize);
				uint16_t file_end_cluster = file_start_cluster + ((file_size + cluster_info.bytes_per_cluster - 1) / cluster_info.bytes_per_cluster);
				// printf("start cluster: %i file size: %i no clusters: %i end cluster: %i\n", file_start_cluster, file_size, file_size/cluster_info.bytes_per_cluster, file_end_cluster);

				//call function to mark the cluster as referenced
				mark_referenced_cluster(cluster_tag, file_start_cluster, file_end_cluster, image_buf, bpb); 
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

    int cluster_tag[cluster_info.number_of_clusters];
    int j = 0;
    for(j; j < cluster_info.number_of_clusters; j++) {
    	cluster_tag[j] = 0;
    }

    follow_dir(cluster_tag, 0, image_buf, bpb);
    find_unreferenced_clusters(cluster_tag, image_buf, bpb);
    find_lost_files(cluster_tag, 2, image_buf, bpb);

    // int i = 0;
    // for(i; i < cluster_info.number_of_clusters; i++) {
    // 	printf("cluster %i: %i referenced: %i\n", i, get_fat_entry(i, image_buf, bpb) ,cluster_tag[i]);
    // }

}