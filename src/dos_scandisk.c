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
#include <ctype.h>

#include "bootsect.h"
#include "bpb.h"
#include "direntry.h"
#include "fat.h"
#include "dos.h"

struct {
	int number_of_clusters;
	int bytes_per_cluster;
} cluster_info;

void get_file_extension(struct direntry *dirent, char *extension) {
	extension[3] = ' ';
	memcpy(extension, dirent->deExtension, 3);

	int i;
	/* remove the spaces from extensions */
    for (i = 3; i > 0; i--) {
	if (extension[i] == ' ') 
	    extension[i] = '\0';
	else 
	    break;
    }
}

void get_file_name(struct direntry *dirent, char *name) {
    name[8] = ' ';
    memcpy(name, &(dirent->deName[0]), 8);

    /* skip over deleted entries */
    if (((uint8_t)name[0]) == SLOT_DELETED) {
    	return;
    }
    
    int i;
	/* remove the spaces from extensions */
    for (i = 8; i > 0; i--) {
	if (name[i] == ' ') 
	    name[i] = '\0';
	else 
	    break;
    }
}

// void find_length_inconsistencies(int cluster_tag[], uint8_t *image_buf, struct bpb33* bpb, char *filename) {
// 	int i;
// 	for(i = 0; i < cluster_info.number_of_clusters; i++) {
// 		if(cluster_tag[i] == -3) {
// 			printf("%s\n", filename);
// 		}
// 	}	
// }

/* write the values into a directory entry */
void write_dirent(struct direntry *dirent, char *filename, uint16_t start_cluster, uint32_t size)
{
    char *p, *p2;
    char *uppername;
    int len, i;

    /* clean out anything old that used to be here */
    memset(dirent, 0, sizeof(struct direntry));

    /* extract just the filename part */
    uppername = strdup(filename);
    p2 = uppername;
    for (i = 0; i < strlen(filename); i++) {
	if (p2[i] == '/' || p2[i] == '\\') {
	    uppername = p2+i+1;
	}
    }

	/* convert filename to upper case */ // LEFT COMMENTED OUT - SPEC SAYS NAME FILES IN LOWERCASE
 //    for (i = 0; i < strlen(uppername); i++) {
	// uppername[i] = toupper(uppername[i]);
 //    }

    /* set the file name and extension */
    memset(dirent->deName, ' ', 8);
    p = strchr(uppername, '.');
    memcpy(dirent->deExtension, "___", 3);
    if (p == NULL) {
	fprintf(stderr, "No filename extension given - defaulting to .___\n");
    } else {
	*p = '\0';
	p++;
	len = strlen(p);
	if (len > 3) len = 3;
	memcpy(dirent->deExtension, p, len);
    }
    if (strlen(uppername)>8) {
	uppername[8]='\0';
    }
    memcpy(dirent->deName, uppername, strlen(uppername));
    free(p2);

    /* set the attributes and file size */
    dirent->deAttributes = ATTR_NORMAL;
    putushort(dirent->deStartCluster, start_cluster);
    putulong(dirent->deFileSize, size);

    /* a real filesystem would set the time and date here, but it's
       not necessary for this coursework */
}

/* create_dirent finds a free slot in the directory, and write the
   directory entry */
void create_dirent(struct direntry *dirent, char *filename, uint16_t start_cluster, uint32_t size, uint8_t *image_buf, struct bpb33* bpb) {

    while(1) {
		if (dirent->deName[0] == SLOT_EMPTY) {
		    /* we found an empty slot at the end of the directory */
		    write_dirent(dirent, filename, start_cluster, size);
		    dirent++;

		    /* make sure the next dirent is set to be empty, just in
		       case it wasn't before */
		    memset((uint8_t*)dirent, 0, sizeof(struct direntry));
		    dirent->deName[0] = SLOT_EMPTY;
		    return;
		}
		if (dirent->deName[0] == SLOT_DELETED) {
		    /* we found a deleted entry - we can just overwrite it */
		    write_dirent(dirent, filename, start_cluster, size);
		    return;
		}
		dirent++;
    }
}

void recover_lost_file(int lost_file_number, uint16_t start_cluster, int cluster_count, uint8_t *image_buf, struct bpb33* bpb) {
	

	struct direntry *dirent;
	dirent = (struct direntry*)cluster_to_addr(0, image_buf, bpb);

	char filename[13]; //max length of filename is 13
	sprintf(filename, "%s%i%s", "found", lost_file_number, ".dat");

    uint32_t file_size = cluster_count * cluster_info.bytes_per_cluster;

	create_dirent(dirent, filename, start_cluster, file_size, image_buf, bpb);
    
}

//part 2 - lists out the lost files
void find_lost_files(int cluster_tag[], uint16_t cluster, uint8_t *image_buf, struct bpb33* bpb) {
	int count = 0;
	int lost_file_start_cluster = -1;
	int total_lost_files = 0;
	int i;
	for(i = cluster; i <= cluster_info.number_of_clusters; i++) {
		int next_cluster = get_fat_entry(i, image_buf, bpb);
		if (cluster_tag[i] == -2) {
			count++;
			if (lost_file_start_cluster == -1) {
				lost_file_start_cluster = i;
			}
			if (is_end_of_file(next_cluster) == 1) {
				total_lost_files++;
				printf("Lost File: %i %i\n", lost_file_start_cluster, count);
				// printf("next call starting on cluster %i\n", cluster+1);
				recover_lost_file(total_lost_files, lost_file_start_cluster, count, image_buf, bpb);
				lost_file_start_cluster = -1;
				count = 0;
			} 
		}
	}
}

//part 1 - find all unreferenced clusters
void find_unreferenced_clusters(int cluster_tag[], uint8_t *image_buf, struct bpb33* bpb) {
	
	int i;
	int found_unreferenced_cluster = 0; //tracks whether any unreferenced cluster is found or not
	for(i = 2; i < cluster_info.number_of_clusters; i++) {
		int next_cluster = get_fat_entry(i, image_buf, bpb);
		if ((next_cluster != 0) && (cluster_tag[i] == 0)) { //if cluster is neither free in the FAT nor used in any file
			if (found_unreferenced_cluster == 0) { //there exists unreferenced clusters in the disk image
				printf("Unreferenced: ");
				found_unreferenced_cluster = 1; //update tracker
			}
			printf("%i ", i);
			cluster_tag[i] = -2;//mark unreferenced cluster with -2 'tag'
		} 
	}
	printf("\n");

}

//mark cluster as referenced
void mark_referenced_cluster(int cluster_tag[], uint16_t cluster, uint16_t end_cluster, uint8_t *image_buf, struct bpb33* bpb) {

	// printf("cluster %i: %i referenced: %i end cluster:%i\n", cluster, get_fat_entry(cluster, image_buf, bpb) ,cluster_tag[cluster], end_cluster);
	if ((cluster == end_cluster) && (is_end_of_file(get_fat_entry(cluster, image_buf, bpb)) == 0)) {
		// printf("NOT END OF FILE: %i", is_end_of_file(cluster));
		cluster_tag[cluster] = -3;
	} else {
		cluster_tag[cluster] = -1; //mark referenced clusters with -1 'tag'
	}

	uint16_t next_cluster = get_fat_entry(cluster, image_buf, bpb);
	if (is_end_of_file(next_cluster) == 1) {
		return;
	} else {
		mark_referenced_cluster(cluster_tag, next_cluster, end_cluster, image_buf, bpb);
	}
}

//walks through directory tree
void follow_dir(int cluster_tag[], uint16_t cluster, uint8_t *image_buf, struct bpb33* bpb, int find_inconsistencies) {
    struct direntry *dirent;
    int d;
    dirent = (struct direntry*)cluster_to_addr(cluster, image_buf, bpb);
    while (1) {
		for (d = 0; d < cluster_info.number_of_clusters; d += sizeof(struct direntry)) {
			// printf("%i\n", cluster);
		    uint16_t file_cluster;
		    // char name[9];
		    // char extension[4];

		    char name[9];
			char extension[4];

			get_file_name(dirent, name);
			get_file_extension(dirent, extension);

			if (name[0] == SLOT_EMPTY)
			return;

		    /* skip over deleted entries */
		    if (((uint8_t)name[0]) == SLOT_DELETED)
			continue;

			// printf("%s\n", name);

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
				follow_dir(cluster_tag, file_cluster, image_buf, bpb, 0);
				cluster_tag[file_cluster] = -1; //mark the clusters referenced by directories with -1 'tag'
		    } else {
				uint16_t file_start_cluster = getushort(dirent->deStartCluster);
				uint32_t file_size = getulong(dirent->deFileSize);
				uint32_t file_end_cluster = file_start_cluster - 1 + ((file_size + cluster_info.bytes_per_cluster - 1) / cluster_info.bytes_per_cluster);
				
				// printf("start cluster: %i file size: %i no clusters: %i end cluster: %i\n", file_start_cluster, file_size, file_size/cluster_info.bytes_per_cluster, file_end_cluster);
				if(find_inconsistencies == 0) {
					//call function to mark the cluster as referenced
					mark_referenced_cluster(cluster_tag, file_start_cluster, file_end_cluster, image_buf, bpb); 
				} else {
					// find_length_inconsistencies(cluster_tag, image_buf, bpb, strcat(strcat(name, "."), extension));
				}
				
				// find_length_inconsistencies(cluster_tag, image_buf, bpb, strcat(strcat(name, "."), extension), file_start_cluster, file_end_cluster);
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
    int j;
    for(j = 0; j < cluster_info.number_of_clusters; j++) {
    	cluster_tag[j] = 0;
    }

    follow_dir(cluster_tag, 0, image_buf, bpb, 0);
    find_unreferenced_clusters(cluster_tag, image_buf, bpb);
    find_lost_files(cluster_tag, 2, image_buf, bpb);

    // follow_dir(cluster_tag, 0, image_buf, bpb, 1);

    int i;
    for(i = 0; i < cluster_info.number_of_clusters; i++) {
    	printf("cluster %i: %i referenced: %i\n", i, get_fat_entry(i, image_buf, bpb) ,cluster_tag[i]);
    }

    free(bpb);

    close(fd);
    exit(0);

}