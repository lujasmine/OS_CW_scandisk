# Operating Systems - scandisk

Year 3 Operating Systems Module Coursework 2

The objective of this coursework is to write the equivalent of scandisk for a DOS (FAT12) filesystem on a floppy disk. Scandisk is a filesystem checker for DOS filesystems. It can check that a filesystem is internally consistent, and corrects any errors found.

TO MAKE: ```make```
TO CLEAN FILES: ```make clean```
TO RUN SHELL: ```./dos_scandisk [disk file image name]```

Implementation description document: [description](src/description.md)

NOTE: machine/endian.h is required in bpb.h in order to be compilable on Mac OSX



