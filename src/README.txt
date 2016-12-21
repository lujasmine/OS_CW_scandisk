COMP 3005 | Operating Systems Coursework 2: scandisk

by Jasmine Lu, 14002484

Makefile:
TO MAKE: make
TO CLEAN FILES: make clean
TO RUN SHELL: ./dos_scandisk [disk image file] e.g. ./dos_scandisk badfloppy2.img

FILE DESCRIPTION: dos_ls.c
Unchanged from original files given in source code on moodle - does a recursive listing of all files and directories in the disk image file.

FILE DESCRIPTION: dos_cp.c
Unchanged from original files given in source code on moodle - copies files into and out of the disk image file.

FILE DESCRIPTION: bootsect.h bpb.h direntry.h fat.h dos.h
Header files containing information about the boot sector, bios parameter block, directory entry, fat and dos as explained in the original spec file.

FILE DESCRIPTION: dos_scandisk.c
Main part of coursework - dos_scandisk finds any unreferenced clusters and the corresponding lost files and creates a directory entry for each lost file named found1.dat, found2.dat etc as specified in the spec. The file also prints out a list of files whose length in the directory entry is inconsistent with their length in the FAT. When an inconsistency in length is found, the program will free any clusters that are beyond the end of a file. The full description of how the code works can be found in description.pdf.

FILE DESCRIPTION: output.jpg output.txt
Output from running dos_scandisk on badfloppy2.img in jpg and txt format. 

