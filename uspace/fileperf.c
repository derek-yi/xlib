
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>  //timer_t

int log_to_file(char *file_path, char *data_ptr, int data_size)
{
    static int file_id = 0;
    int fd;
    char name_buff[64];

    if (data_ptr == NULL) {
        return -1;
    }

    if (file_path) sprintf(name_buff, "%slog%d.dat", file_path, file_id++);
    else sprintf(name_buff, "log%d.dat", file_id++);
    
    fd = open(name_buff, O_CREAT|O_RDWR, 0666);
    if (fd < 0) {
        return -1;
    }

    if ( write(fd, data_ptr, data_size) < data_size ) {
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

#define MAX_BLK_SIZE    102400
char data_buff[MAX_BLK_SIZE];

int main(int argc, char **argv)
{
    int ret;
    struct timeval t_start, t_end;
    int t_used = 0;
    int wr_cnt, blk_size;
    char *file_path = NULL;

    if (argc < 4) {
        printf("usange: bin <wr_cnt> <blk_size> [<path>]\n");
        return 0;
    }

    wr_cnt = atoi(argv[1]);
    blk_size = atoi(argv[2]);
    if (blk_size > MAX_BLK_SIZE) blk_size = MAX_BLK_SIZE;
    if (argc > 3) file_path = argv[3];
    
    for(int i = 0; i < blk_size; i++) {
        data_buff[i] = (char)(i%256);
    }
    
    gettimeofday(&t_start, NULL);
    for(int i = 0; i < wr_cnt; i++) {
        ret = log_to_file(file_path, data_buff, blk_size);
        if (ret != 0) {
            printf("log_to_file failed\n");
            break;
        }
    }
    gettimeofday(&t_end, NULL);
    
    t_used = (t_end.tv_sec - t_start.tv_sec)*1000000+(t_end.tv_usec - t_start.tv_usec);//us
    t_used = t_used/1000; //ms
    printf("t_used = %d\n", t_used);

    return 0;
}

