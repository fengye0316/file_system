#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "file_system.h"

int firstEmpty (int *blocks) {
    int i;
    for (i = 0; i < BLOCK_SIZE; i++) {
        if (blocks[i] == 0) {
            return i;
        }
    }
    return -1;
}

int write_file (disk_t disk, char *file_name) {
    FILE *file = fopen (file_name, "r");

    if (file == NULL) {
        fflush (NULL);
        fprintf (stderr, "%s: %s\n", file_name, strerror (errno));
        fflush (NULL);

        return -1;
    } else {
        unsigned char *databuf = calloc (BLOCK_SIZE, sizeof (unsigned char));
        unsigned char *filebuf = calloc (BLOCK_SIZE * disk->size, sizeof (unsigned char));

        int i = 0;

        while (feof (file) == 0)
             filebuf[i++] = fgetc(file);
        filebuf[i - 1] = '\0';

        int num_blocks = ceil ((double) i / BLOCK_SIZE);

        fclose (file);
        printf ("\nFinish reading file\n");

        int *retublocks = read_block_map(disk);
        int i_node_block = firstEmpty (retublocks);

        retublocks[i_node_block] = 1;
        write_block_map (disk, retublocks);

        int file_block;
        int num_pointers = num_blocks + 1;
        int pointers[num_pointers];
        pointers[num_pointers - 1] = '\0';

        for (i = 0; i < num_blocks; i++) {
            retublocks = read_block_map(disk);
            file_block = firstEmpty (retublocks);
            pointers[i] = file_block;

            retublocks[file_block] = 1;
            write_block_map (disk, retublocks);
        }

        printf("\nWriting Inode\n");

        Inode *node = writeInode(disk, i_node_block, pointers, false, file_name);
        Inode *root = readInode (disk, 1);

        int size = arrayLength(root->pointers) + 1;
        printf("%d\n", size);
        int *ptrs = calloc (size + 1, sizeof (int));
        for (i = 0; i < size - 1; i++) {
            ptrs[i] = root->pointers[i];
        }

        ptrs[i++] = i_node_block;
        ptrs[i] = '\0';
        root->pointers = ptrs;

        int k = 0;
        for(i = 0; i < num_blocks; ++i, k = k + BLOCK_SIZE) {
            strncpy (databuf, filebuf + k, BLOCK_SIZE);
            writeblock (disk, pointers[i], databuf);
        }

        printf("Wrote successfully\n");

        free (filebuf);
        free (databuf);
        return i_node_block;
    }
}

void readdisk(disk_t disk, int blocknum){
    Inode *file = readInode(disk,blocknum);
    if(file->size == 0) {
        printf("Is a directory\n");
        return;
    }
    unsigned char *databuf = malloc(disk->block_size * (sizeof(unsigned char)));
    unsigned char *filebuf = malloc(disk->block_size * file->size * sizeof(unsigned char));
    int i,j,k;
    k = 0;
    for(i=0;i<file->size;++i,++blocknum){
        readblock(disk,file->pointers[i],databuf);
        for(j=0;j<(disk->block_size * sizeof(unsigned char));++j,++k){
            filebuf[k] = databuf[j];
        }
    }
    for(i=0;i<disk->block_size * file->size * sizeof(unsigned char)
            || filebuf[i] == '\0';++i){
        putchar(filebuf[i]);
    }
    freeInode (file);
}

void main(int argc, char *argv[])
{
    char *disk_name;
    disk_t disk;
    unsigned char *databuf;
    int i, j;

    // Read the parameters
    if(argc != 2) {
        printf("Usage: testdisk <disk_name>\n");
        exit(-1);
    }

    disk_name = (char *)argv[1];

    // Open the disk
    disk = opendisk(disk_name);

    //Read and write super block
    write_super_block(disk);
    read_super_block(disk);

    // Write the root directory
    write_root_dir (disk);

    // Set up a buffer for writing and reading
    databuf = malloc(disk->block_size);

    //test writing block map
    printf("testing read and write block map\n");
    int *bmap = calloc (7, disk->size);

    for (i = 0; i < 7; i++) {
        bmap[i] = 1;
    }

    write_block_map(disk,bmap);
    int * retublocks = read_block_map(disk);
    for(i = 0; i < 7; i+=1){
        printf("%d, ",retublocks[i]);
    }

    printf("\n");
    free(retublocks);

    char buffer[1000];

    int file_num;
    do {
        printf ("Enter file name to copy to disk: ");
        char *line = fgets (buffer, sizeof buffer, stdin);
        line = strchr (buffer, '\n');

        while (line == NULL || buffer[0] == '\0') {
            line = fgets (buffer, sizeof buffer, stdin);
            line = strchr (buffer, '\n');
        }
        *line = '\0';
        char *file_name = strdup (buffer);

        file_num = write_file (disk, file_name);
    } while (file_num == -1);

    do {
        printf ("Enter file name to print out: ");
        char *line = fgets (buffer, sizeof buffer, stdin);
        line = strchr (buffer, '\n');

        while (line == NULL || buffer[0] == '\0') {
            line = fgets (buffer, sizeof buffer, stdin);
            line = strchr (buffer, '\n');
        }
        *line = '\0';
        char *file_name = strdup (buffer);

        Inode *root = readInode (disk, 1);

        for (i = 0; i < arrayLength (root->pointers); i++) {
            Inode* node = readInode (disk, root->pointers[i]);

            if (strcmp (file_name, node->name) == 0) {
                file_num = i;
                break;
            }
            file_num = -1;
        }

        if (file_num != -1) {
            readdisk (disk, file_num);
        }
    } while (file_num == -1);

    free(disk);
    free(databuf);
    exit(0);
}
