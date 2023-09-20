#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "simplefs.h"
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h> 


void test_4() {

    time_t t;
    srand( (unsigned) time(&t) );

    struct timeval  start, finish;
    suseconds_t runtime = 0;
    

    sfs_mount("sa");

    sfs_mount ("sa");

    char fname[10] = "filex.bin";
    char buf[4096];

    int k, fd;
    for (k = 0; k < 4096; k++) {
        buf[k] = 'x';
    }

    
    int j;
    for ( j = 0; j < 32; j++ )
    {
        fname[4] = j + '0';
        sfs_create(fname);
        fd = sfs_open(fname, 1);

        for (k = 0; k < 1024; k++) 
        {
            sfs_append(fd, buf, 4096);
        }
        sfs_close(fd);
    }

    j -= 1;
    fname[4] = j + '0';
    //printf("%s\n",fname);
    sfs_delete(fname);
    
    char fname2[10] = "fxxx";
    sfs_create(fname2);
    fd = sfs_open(fname2, 1);

    gettimeofday( &start, NULL );
    for (k = 0; k < 1024; k++) 
    {
        sfs_append(fd, buf, 4096);
    }
    gettimeofday( &finish, NULL );


    sfs_umount("sa");

    runtime = finish.tv_sec * 1000000 + finish.tv_usec - start.tv_sec * 1000000 - start.tv_usec;

    printf("Time: %ld\n", runtime);
}

int main(int argc, char **argv)
{
    int ret;

    ret  = create_format_vdisk ("sa", 28); 
    if (ret != 0) {
        printf ("there was an error in creating the disk\n");
        exit(1); 
    }
    test_4();

    //end = clock();
    // time = (int) ((double) (end - start)) / (CLOCKS_PER_SEC / 1000000.0); // in milliseconds
}



void test_1() {
    int ret;

    printf ("started\n");
    ret = sfs_mount ("sa");
    if (ret != 0) {
        printf ("could not mount \n");
        exit (1);
    }



    printf ("creating files\n"); 
    printf("\nSAAAAAAAAAAA %d\n", get_empty_count());
    sfs_create ("file1.bin");
    sfs_create ("file2.bin");
    sfs_create ("file3.bin");
    printf("\nSAAAAAAAAAAA %d\n", get_empty_count());
    
    
    int fd = sfs_open("file3.bin", 1);
    printf("\nSAAAAAAAAAAA %d\n", get_empty_count());
    char wbuf[10] = "hello";
    int wn = sfs_append(fd, wbuf, 5); 
    printf("\nSAAAAAAAAAAA %d\n", get_empty_count());
    printf("wn: %d\n", wn);
    sfs_close(fd);
    
    
    // int n = sfs_read(fd, buf, 5);
    
    // printf("n: %d, %s\n", n, buf);

    sfs_delete("file2.bin");

    sfs_create ("file4.bin");
    sfs_create ("file5.bin");
    printf("\nSAAAAAAAAAAA %d\n", get_empty_count());
    char sa[5000];
    int x;
    for (x = 0; x < 5000; x++) {
        sa[x] = 'x';
    }
    fd = sfs_open("file4.bin", 1);
    sfs_append(fd, sa, 5000);
    sfs_append(fd, wbuf, 5);
    int fd2 = sfs_open("file3.bin", 1);
    sfs_append(fd2, sa, 5000);
    sfs_append(fd2, sa, 5000);
    sfs_append(fd2, wbuf, 5);
    // sfs_append(fd, sa, 4096);
    /*
    sfs_append(fd, sa, 4096);
    sfs_append(fd, sa, 4096);
    sfs_append(fd, sa, 583);
    
    sfs_read(fd, sa, 4096);
    */
    sfs_close(fd);
    sfs_close(fd2);

    
    fd2 = sfs_open("file3.bin", 0);
    fd = sfs_open("file4.bin", 0);
    sfs_read(fd, sa, 4998);
    char buf[50];
    sfs_read(fd, buf, 5);
    sfs_read(fd, buf, 5);
    buf[5] = '\0';
    sfs_read(fd2, sa, 10005);
    printf("FINAL: %s\n", buf);
    char buf2[50];
    sfs_read(fd2, buf2, 5);
    buf[5] = '\0';
    printf("FINAL: %s\n", buf2);
    print_files();
    
    sfs_close(fd);
    fd = sfs_open("file1.bin", 1);

    printf("\nSAAAAAAAAAAA %d\n", get_empty_count());
    char y[4096];
    int k;
    for (k = 0; k < 1023; k++) {
        sfs_append(fd, y, 4096);
    }
    sfs_append(fd, y, 4096);
    sfs_umount("sa");
    exit(0);
}

void test_3() {
    printf("starting\n");
    sfs_mount ("sa");
    //char w1[10] = "sa";
    char w2[10] = "hello";

    char sa[10000];
    char sar[10000];
    int i;
    for (i = 0; i < 10000; i++) {
        sa[i] = 'x';
        sar[i] = 'x';
    }

    printf("buffers allocated\n");

    sfs_create("file1");
    int fd1 = sfs_open("file1", 1);

    sfs_append(fd1, sa, 8190);
    printf("append1\n");
    sfs_append(fd1, w2, 5);
    sfs_append(fd1, sa, 1000);
    printf("append2\n");

    sfs_close(fd1);
    printf("appending done\n");

    sfs_open("file1", 0);
    char r1[10];
    sfs_read(fd1, sar, 8192);
    sfs_read(fd1, r1, 5);

    printf("reading done\n");

    r1[5] = '\0';
    printf("%s\n", r1);
    
    

    sfs_umount("sa");
    printf("ending\n");
}

void test_2() {
    sfs_mount ("sa");

    char fname[10] = "filex.bin";
    char buf[4096];

    int k, fd, i;
    for (k = 0; k < 4096; k++) {
        buf[k] = 'x';
    }

    for (k = 0; k < 64; k++) {
        fname[4] = k + '0';
        sfs_create(fname);
        fd = sfs_open(fname, 1);
        printf("SA %d\n", fd);
        for (i = 0; i < 1022; i++) {
            sfs_append(fd, buf, 4096);
        }
        printf("%d %d\n", k, get_empty_count());
        sfs_close(fd);
        
        if (k == 7) {
            sfs_delete(fname);
        }
    }

    fd = sfs_open("file5.bin", 0);
    char x[10];
    for (k = 0; k < 1026; k++) {
        sfs_read(fd, buf, 4096);
    }
    x[9] = '\0';
    printf("\n\n%s\n", x);
    
    print_files();
    
    
}
