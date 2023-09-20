#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "simplefs.h"

// TODO list:
// mode checks - done?
// multiple block tests - done?
// full memory checks / decrease empty count

// structs
struct superblock {
    int size;
    int empty_count;
    int b_count;
    char filler[4084];
};

struct dentry {
    char filename[110];
    int fid;
    int used;
    char filler[8];
};

struct fcb {
    int size;
    int start;
    int used;
    int mode; // 0->read, 1->append, -1->not opened yet
    int open_count;
    char filler[108];
};

struct open_file {
    int fid;
    int mode; // 0->read, 1->append
    int at;
};

// Global Variables =======================================
int vdisk_fd; // Global virtual disk file descriptor. Global within the library.
              // Will be assigned with the vsfs_mount call.
              // Any function in this file can use this.
              // Applications will not use  this directly. 
int b_count; // block count
void *temp_block;
struct open_file open_table[16];
int open_count;
// ========================================================

void print_intarr(int *arr, int n) {
    int i;
    printf("\n");
    for (i = 0; i < n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

void print_char_bits(char c) {
    int i;
    for (i=0; i < 8; i++) {
        unsigned char bit = (c >> i) & 1;
        printf("%u", bit);
    }
}

// read block k from disk (virtual disk) into buffer block.
// size of the block is BLOCKSIZE.
// space for block must be allocated outside of this function.
// block numbers start from 0 in the virtual disk. 



int read_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = read (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
        printf ("read error, %d\n", n);
        return -1;
    }
    return (0); 
}

// write block k into the virtual disk. 
int write_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = write (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
        printf ("write error\n");
        return (-1);
    }
    return 0; 
}

void print_files() {
    int i, j, fid, q;
    struct dentry *de = malloc(BLOCKSIZE);
    struct fcb *fcbs = malloc(BLOCKSIZE);
    
    for (i = 5; i <= 8; i++) {
        read_block(de, i);
        for (j = 0; j < 32; j++) {
            if (de[j].used) {
                fid = de[j].fid;
                //printf("filename: %s, fid: %d,", de[j].filename, fid);
                q = (fid/32) + 9;
                //r = fid % 32;
                read_block(fcbs, q);
                //printf(" size: %d\n", fcbs[r].size);
            }
        }
    }

    free(de);
    free(fcbs);
}

void print_block(void *block) {
    char *arr = (char *) block;
    int i;

    //printf("\n");
    for (i = 0; i < BLOCKSIZE; i++) {
        char c = arr[i];
        print_char_bits(c);
        if (i == 5) {
            break;
        }
    }
    //printf("\n");    
}

int read_bit(void *block, int i) {
    int q = i / 8;
    int r = i % 8;

    unsigned char *arr = (unsigned char *) block;
    unsigned char bit_i = 1 << r;

    return bit_i & arr[q];
}

void write_bit(void *block, int i, int bit) {
    int q = i / 8;
    int r = i % 8;

    unsigned char *arr = (unsigned char *) block;
    unsigned char bit_i = 1 << r;

    if (bit) {
        arr[q] = arr[q] | bit_i;
    } else {
        arr[q] = arr[q] & (~bit_i);
    }
}

void alloc_n(int n, int *bids) {
    int alloced_count = 0;
    int bid = 0;
    int k;
    int i;
    int len = 4096*8;
    int bit;
    for (k = 1; k <= 4; k++) {
        read_block(temp_block, k);
        for (i = 0; i < len; i++) {
            bit = read_bit(temp_block, i);
            if (!bit) {
                write_bit(temp_block, i, 1);
                bids[alloced_count] = bid;
                alloced_count++;
            }
            bid++;
            if (alloced_count >= n) {
                break;
            }
        }
        write_block(temp_block, k);
        if (alloced_count >= n) {
            break;
        }
    }
}

void free_block(int bid) {
    int len = 4096*8;
    int n = (bid / len) + 1;
    int i = bid % len;
    read_block(temp_block, n);
    write_bit(temp_block, i, 0);
    write_block(temp_block, n);
}

void free_blocks(int *blocks, int n) {
    int i;
    int bid;
    // TODO: maybe free all bids in the same block at once for efficiency?
    for (i = 0; i < n; i++) {
        bid = blocks[i];
        free_block(bid);
    }
}

struct superblock *get_sb() {
    struct superblock *sb = malloc(sizeof(struct superblock));
    read_block((void *) sb, 0);
    return sb;
}

int alloc_block() {
    int bid = 0;
    int k;
    int i;
    int len = 4096*8;
    int bit;
    for (k = 1; k <= 4; k++) {
        read_block(temp_block, k);
        for (i = 0; i < len; i++) {
            bit = read_bit(temp_block, i);
            if (!bit) {
                write_bit(temp_block, i, 1);
                break;
            }
            bid++;
        }
        if (!bit) {
            write_block(temp_block, k);
            break;
        }
    }

    return bid;
}

int alloc_fcb() {
    int i, j;

    struct fcb *fcbs = malloc(sizeof(struct fcb)*32);
    for (i = 9; i <= 12; i++) {
        read_block((void *) fcbs, i);

        for (j = 0; j < 32; j++) {
            if (!fcbs[j].used) {
                break;
            }
        }
        if (!fcbs[j].used) {
            fcbs[j].mode = -1;
            fcbs[j].size = 0;
            fcbs[j].start = alloc_block();
            fcbs[j].used = 1;
            // printf("allocating: %d, %d, bid: %d\n", i, j, fcbs[j].start);
            
            write_block((void *) fcbs, i);
            break;
        }
    }

    free(fcbs);
    return (i-9)*32 + j;
}

void set_empty_count(int value) {
    struct superblock *sb = malloc(BLOCKSIZE);
    read_block((void *) sb, 0);
    sb->empty_count = value;
    write_block(sb, 0);
    free(sb);
}

int get_empty_count() {
    struct superblock *sb = malloc(BLOCKSIZE);
    read_block((void *) sb, 0);
    int empty_count = sb->empty_count;
    free(sb);
    return empty_count;
}

/**********************************************************************
   The following functions are to be called by applications directly. 
***********************************************************************/

// this function is partially implemented.
int create_format_vdisk (char *vdiskname, unsigned int m)
{
    char command[1000];
    int size;
    int num = 1;
    int count;
    size  = num << m;
    count = size / BLOCKSIZE;
    //    printf ("%d %d", m, size);
    sprintf (command, "dd if=/dev/zero of=%s bs=%d count=%d",
             vdiskname, BLOCKSIZE, count);
    //printf ("executing command: %s\n", command);
    system (command);

    // now write the code to format the disk below.
    // .. your code...
    vdisk_fd = open(vdiskname, O_RDWR); 


    void *block;
    // init superblock
    struct superblock *sb = malloc(sizeof(struct superblock));
    sb->empty_count = count - 13;
    sb->b_count = count;
    sb->size = size;
    block = (void*) sb;
    write_block(block, 0);
    free(sb);

    // init bitmap
    int i;
    unsigned char c = 255;
    unsigned char *bmap = malloc(sizeof(char)*BLOCKSIZE);
    bmap[0] = c;
    c = 31;
    bmap[1] = c;
    write_block((void *) bmap, 1);
    free(bmap);

    // init dir entries
    struct dentry *de = malloc(sizeof(struct dentry)*32);
    for (i = 0; i < 32; i++) {
        de[i].used = 0;
    }
    block = (void *) de;
    for (i = 5; i <= 8; i++) {
        write_block(block, i);
    }
    free(de);

    // init FCBs
    struct fcb *fcb = malloc(sizeof(struct fcb)*32);
    for (i = 0; i < 32; i++) {
        fcb[i].used = 0;
    }
    block = (void *) fcb;
    for (i = 9; i <= 12; i++) {
        write_block(block, i);
    }
    free(fcb);

    return 0;
}


// already implemented
int sfs_mount (char *vdiskname)
{
    // simply open the Linux file vdiskname and in this
    // way make it ready to be used for other operations.
    // vdisk_fd is global; hence other function can use it. 
    vdisk_fd = open(vdiskname, O_RDWR); 
    struct superblock *sb = get_sb();
    b_count = sb->b_count;
    free(sb);
    temp_block = malloc(sizeof(char)*4096);

    int i;
    for (i = 0; i < 16; i++) {
        open_table[i].fid = -1;
    }
    open_count = 0;
    return(0);
}


// already implemented
int sfs_umount ()
{
    free(temp_block);
    fsync (vdisk_fd); // copy everything in memory to disk
    close (vdisk_fd);
    return (0); 
}


int sfs_create(char *filename)
{
    int empty_count = get_empty_count();
    if (empty_count <= 1) {
        //printf("No empty space to create file\n");
        return -1;
    }

    int i, j;
    int flag = 0;
    struct dentry *de = malloc(sizeof(struct dentry)*32);
    for (i = 5; i <= 8; i++) {
        read_block((void *) de, i);
        for (j = 0; j < 32; j++) {
            if (!de[j].used) {
                flag = 1;
                break;
            }
        }
        if (flag && !de[j].used) {
            // printf("Writing file at %d %d \n", i, j);
            strcpy(de[j].filename, filename); 
            de[j].fid = alloc_fcb();
            de[j].used = 1;
            write_block((void *) de, i);
            break;
        }
    }

    free(de);

    set_empty_count(empty_count-1);
    return (0);
}


int sfs_open(char *file, int mode)
{
    if (open_count >= 16) {
        return -1;
    }
    //printf("opening: %s\n", file);

    int i, j;
    int flag = 0;
    struct dentry *de;
    for (i = 5; i <= 8; i++) {
        read_block(temp_block, i);
        de = (struct dentry *) temp_block;

        for (j = 0; j < 32; j++) {
            if (de[j].used) {
                //printf("%s\n", de[j].filename);
            }
            if (de[j].used && !strcmp( de[j].filename, file)) {
                flag = 1; 
                break;
            }
        }
        if (flag) {
            break;
        }
    }

    if (!flag) {
        printf("XXx\n");
        return -1;
    }

    // printf("FILENAMES: %s-%s", file, de[j].filename);
    struct fcb *fcbs = malloc(BLOCKSIZE);
    int q = (de[j].fid/32) + 9;
    int r = (de[j].fid%32);
    read_block(fcbs, q);
    int fcb_mode = fcbs[r].mode;
    // printf(" mode: %d\n", fcb_mode);
    if (fcb_mode != -1){
        if (fcb_mode != mode) {
            //printf("WARNING: %s previously opened in mode %d but now tried %d...\n", file, fcb_mode, mode);
            return -1;
        }
    }
    fcbs[r].mode = mode;
    fcbs[r].open_count++;
    write_block(fcbs, q);
    free(fcbs);

    for (i = 0; i < 16; i++) {
        if (open_table[i].fid == -1) {
            open_table[i].fid = de[j].fid;
            open_table[i].mode = mode;
            open_table[i].at = 0;
            open_count++;
            return i;
        }
    }
    //printf("ERROR000\n");
    return -1; 
}

int sfs_close(int fd){
    int fid = open_table[fd].fid;
    if (fid == -1) {
        //printf("WARNING: file does not exist could not close\n");
        return -1;
    }

    struct fcb *fcbs = malloc(BLOCKSIZE);
    int q = (fid/32) + 9;
    int r = (fid%32);
    read_block(fcbs, q);

    fcbs[r].open_count--;
    if (fcbs[r].open_count == 0) {
        fcbs[r].mode = -1;
    }
    write_block(fcbs, q);
    free(fcbs);

    open_table[fd].fid = -1;

    open_count--;
    return (0); 
}

int sfs_getsize (int  fd)
{
    int fid = open_table[fd].fid;
    if (fid == -1) {
        return -1;
    }
    int q = (fid / 32) + 9;
    int r = fid % 32;

    struct fcb *fcbs;
    read_block(temp_block, q);
    fcbs = (struct fcb *) temp_block;

    return fcbs[r].size; 
}

int sfs_read(int fd, void *buf, int n){
    int fid = open_table[fd].fid;
    if (fid == -1) {
        //printf("WARNING: invalid fd in sfs_read, returning -1...\n");
        return -1;
    }
    if (open_table[fd].mode == 1) {
        //printf("WARNING: cannot read in append mode, returning -1...\n");
        return -1;
    }
    int q = (fid / 32) + 9;
    int r = fid % 32;

    struct fcb *fcbs;
    read_block(temp_block, q);
    fcbs = (struct fcb *) temp_block;
    int size = fcbs[r].size;
    int *indices = malloc(sizeof(int)*1024);
    read_block((void *) indices, fcbs[r].start);
    // printf("size: %d, index_bid: %d, first_index: %d\n", size, fcbs[r].start, indices[0]);

    int at = open_table[fd].at;
    // printf("at %d\n", at);
    if (at >= size) {
        //printf("WARNING: at already size, returning 0...\n");
        return 0;
    }
    q = at / BLOCKSIZE;
    r = at % BLOCKSIZE;

    char *to, *from;
    
    from = (char *) temp_block;
    to = (char *) buf;

    read_block(temp_block, indices[q]);
    // printf("from first char: %c, reading from block %d...\n", from[0], indices[q]);
    int i;

    for (i = 0; i < n; i++) {
        if (r >= BLOCKSIZE) {
            q++;
            r = 0;
            read_block(temp_block, indices[q]);
            from = (char *) temp_block;
        }
        to[i] = from[r];
        r++;
        at++;
        if (at >= size) {
            open_table[fd].at = at;
            return i + 1;
        }
    }
    open_table[fd].at = at;

    free(indices);
    return i; 
}


int sfs_append(int fd, void *buf, int n)
{
    if (fd < 0 || fd > 15) {
        return -1;
    }
    int fid = open_table[fd].fid;
    if (fid == -1) {
        //printf("WARNING: invalid fd in sfs_append, returning -1...\n");
        return -1;
    }
    if (open_table[fd].mode == 0) {
        //printf("WARNING: cannot append in read mode, returning -1...\n");
        return -1;
    }
    int q = (fid / 32) + 9;
    int r = fid % 32;

    struct fcb *fcbs = malloc(sizeof(struct fcb)*32);
    read_block(fcbs, q);
    // printf("fid: %d, q: %d, r: %d\n", fid, q, r);
    int index_bid = fcbs[r].start;
    int *indices = malloc(sizeof(int)*1024);
    int size = fcbs[r].size;
    read_block(indices, index_bid);
    // printf("size: %d, index_bid: %d\n", size, index_bid);

    int at = size;
    q = at / BLOCKSIZE;
    r = at % BLOCKSIZE;

    int empty_count = get_empty_count();

    if (size >= 4096*1024) {
        //printf("SA\n");
        return 0;
    }

    if (size == 0) {
        if (empty_count == 0) {
            return 0;
        }
        indices[q] = alloc_block();
        empty_count--;
        // printf("size was 0, allocating initial block %d...\n", indices[q]);
    }
    char *block = malloc(BLOCKSIZE);
    char *from = (char *) buf;
    read_block(block, indices[q]);
    int i;
    for (i = 0; i < n; i++) {
        // printf("writing %c to block %d byte %d...\n", from[i], indices[q], r);
        block[r] = from[i];
        r++;
        if (r % BLOCKSIZE == 0) {
            write_block(block, indices[q]);
            r = 0;
            q++;
            if (empty_count == 0) {
                //printf("WARNING: no free blocks left while appending...\n");
                i++;
                fcbs[fid % 32].size += i;
                write_block((void *) fcbs, (fid / 32) + 9);
                write_block(indices, index_bid);
                set_empty_count(0);
                free(block);
                free(fcbs);
                free(indices);
                return i;
            } else {
                if (q > 1023) {
                    //printf("WARNING: index block full...\n");
                    i++;
                    fcbs[fid % 32].size += i;
                    write_block((void *) fcbs, (fid / 32) + 9);
                    write_block(indices, index_bid);
                    set_empty_count(empty_count);
                    free(block);
                    free(fcbs);
                    free(indices);
                    return i;
                }
                if (empty_count != 0) {
                    empty_count--;
                    indices[q] = alloc_block();
                }
            }
        }
    }
    // printf("block first char: %c, writing at block %d...\n", (char) block[0], indices[q]);
    write_block((void *) block, indices[q]);
    write_block((void *) indices, index_bid);
    fcbs[fid % 32].size += i;
    write_block((void *) fcbs, (fid / 32) + 9);
    set_empty_count(empty_count);
    free(block);
    free(fcbs);
    free(indices);
    return i; 
}

int sfs_delete(char *filename)
{
    int i, j;
    struct dentry *de = malloc(BLOCKSIZE);
    for (i = 5; i <= 8; i++) {
        read_block(de, i);

        for (j = 0; j < 32; j++) {
            if (!strcmp(de[j].filename, filename)) {
                //printf("deleting file at %d %d\n", i, j);
                de[j].used = 0;
                int fid = de[j].fid;
                int q = fid / 32;
                int r = fid % 32;

                struct fcb *fcbs = malloc(sizeof(struct fcb)*32);
                read_block((void *) fcbs, 9+q);
                fcbs[r].used = 0;

                // free blocks
                int size = fcbs[r].size;
                int bcount = (size / 4096) + 1;
                if (size == 0) {
                    bcount = 0;
                }
                if (size > 0) {
                    int index_bid = fcbs[r].start;
                    int *indices = malloc(sizeof(int)*1024);
                    read_block((void *) indices, index_bid);
                    // print_intarr(indices, 1024);
                    free_blocks(indices, bcount);
                    ////printf("XX: %d\n", bcount);
                    indices[0] = index_bid;
                    free_blocks(indices, 1);
                    free(indices);
                }
                // free fcb
                write_block((void *) fcbs, 9+q);
                write_block((void *) de, i);
                free(fcbs);
                free(de);
                // free dentry
                
                // printf("XX: %d\n", get_empty_count());
                set_empty_count(get_empty_count() + bcount + 1);
                // printf("XX: %d\n", get_empty_count());

                return 0;
            }
        }
    }
    free(de);
    return -1; 
}

