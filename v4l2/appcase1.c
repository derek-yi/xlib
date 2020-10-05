

//https://github.com/willxin1024/CamWatcher/blob/master/device.c

#include "device.h"
#include "print.h"
#include "write_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define FLUSH_NUM 1
void suc_err(int res, char* str)
{
    if (res < 0)
    {
        fprintf(stderr, "%s error: %s\n",str,strerror(errno));
        exit(1);
    }

}
void init_fmt()
{
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//������������
    fmt.fmt.pix.width = 320;//ͼ��Ŀ��
    fmt.fmt.pix.height = 240;//ͼ��ĸ߶�
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;//��ɫ�ռ�
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    ioctl(camera_fd, VIDIOC_S_FMT, &fmt);
    ERR_PUTS("format");
}
void init_mmap()
{

    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = FLUSH_NUM;//��������
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
     ioctl(camera_fd, VIDIOC_REQBUFS, &req);
    ERR_PUTS( "Req_bufs");

    buffer = calloc(req.count, sizeof(Videobuf));
    struct v4l2_buffer buf;
    for (bufs_num = 0; bufs_num < req.count; bufs_num++)
    {
        memset(&buf, 0, sizeof(buf));
        buf.index = bufs_num;//���û���������
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.field = V4L2_FIELD_INTERLACED;
        buf.memory = V4L2_MEMORY_MMAP;
        //��ȡ������Ϣ
         ioctl(camera_fd, VIDIOC_QUERYBUF, &buf);
        ERR_PUTS( "Query_buf");
        //���û����С
        buffer[bufs_num].length = buf.length;
        //�ڶѿռ��ж�̬�����������ռ�
        tmp_buf = (unsigned char*)calloc(buffer[okindex].length, sizeof(char));
        //���豸�ļ��ĵ�ַӳ�䵽�û��ռ�������ַ
        buffer[bufs_num].start = mmap(NULL,
                                      buf.length,
                                      PROT_READ | PROT_WRITE,
                                      MAP_SHARED,
                                      camera_fd,
                                      buf.m.offset);
        if (buffer[bufs_num].start == MAP_FAILED)
            write_log(3, "init_mmap()","mmap failed.");
        else
            write_log(1, "init_mmap()","mmap success.");

    }
}


int get_dev_info()
{
    //��ȡ��ǰ�豸������
    struct v4l2_capability cap;
    ioctl(camera_fd, VIDIOC_QUERYCAP, &cap);
    ERR_PUTS( "get cap");
    //��ȡ��ǰ�豸�������ʽ
    struct v4l2_format fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
     ioctl(camera_fd, VIDIOC_G_FMT, &fmt);
    ERR_PUTS( "get format");
    //��ȡ��ǰ�豸��֡��
    struct v4l2_streamparm parm;
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
     ioctl(camera_fd, VIDIOC_G_PARM, &parm);
    ERR_PUTS( "get parm");
    //��ӡ����豸��Ϣ��
    printf("----------------dev_info---------------\n");
    printf("driver:	%s\n", cap.driver);
    printf("card:	%s\n", cap.card); //����ͷ���豸��
    printf("bus:	%s\n", cap.bus_info);
    printf("width:	%d\n", fmt.fmt.pix.width); //��ǰ��ͼ��������
    printf("height:	%d\n", fmt.fmt.pix.height); //��ǰ��ͼ������߶�
    printf("FPS:	%d\n", parm.parm.capture.timeperframe.denominator);
    printf("------------------end------------------\n");

    return 0;
}

int cam_on()
{
    //ͨ��v4l2������ͷ
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(camera_fd, VIDIOC_STREAMON, &type)<0)
    {
        ERR_PUTS( "camera on");
        write_log(3,"cam_on()","camera open failed");
    }

    //����һ�λ���ˢ��
    struct v4l2_buffer buf;
    int i;
    for (i = 0; i < bufs_num; i++)
    {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if(ioctl(camera_fd, VIDIOC_QBUF, &buf)<0)
        {
            ERR_PUTS( "Q_buf_init");
            write_log(3,"cam_on()","Q_buf_init failed");

        }

    }
    return 0;
}

int cam_off()
{
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(camera_fd, VIDIOC_STREAMOFF, &type);
    on_off = 0;
    ERR_PUTS( "close stream");
    write_log(2,"cam_off","camera has been closed");
    return 0;
}

//����ͼ��
int get_frame(void)
{
    struct v4l2_buffer buf;

    counter++;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    fd_set	readfds;
    FD_ZERO(&readfds);
    FD_SET(camera_fd, &readfds);
    struct timeval tv;//�����豸��Ӧʱ��
    tv.tv_sec = 1;//��
    tv.tv_usec = 0;//΢��
    while (select(camera_fd + 1, &readfds, NULL, NULL, &tv) <= 0)
    {
        fprintf(stderr, "camera busy,Dq_buf time out\n");
        FD_ZERO(&readfds);
        FD_SET(camera_fd, &readfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
    }
     ioctl(camera_fd, VIDIOC_DQBUF, &buf);
    ERR_PUTS( "Dq_buf");

    //buf.index��ʾ�Ѿ�ˢ�ºõĿ��õĻ���������
    okindex = buf.index;
    //���»������ô�С
    buffer[okindex].length = buf.bytesused;
    //��n�β���ͼƬ:(��n��ˢ�������������-��n�����汻ˢ��)
    ////printf("Image_%03d:(%d-%d)\n",counter,counter / bufs_num,okindex);


    //��ͼ����뻺�������(����)
     ioctl(camera_fd, VIDIOC_QBUF, &buf);
    ERR_PUTS( "Q_buf");

    return 0;
}


void install_dev()
{
    init_fmt();
    init_mmap();
    write_log(1,"install_dev()","fmt & mmap has been inited.");

}

void uninstall_dev()
{

    int i;
    for (i = 0; i < bufs_num; ++i)
    {
         munmap(buffer[i].start, buffer[i].length);
        ERR_PUTS( "munmap");
    }
    free(buffer);
    free(tmp_buf);
    close(camera_fd);
}

#if 1

#include "device.h"
#include "server.h"
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

void  sig_handler(int signo)
{
    if(signo == SIGPIPE)
    {
        printf("Recv SIGPIPE From Browser.\n");
        write_log(1, "sig_handler()", "Recv SIGPIPE From Browser.");
    }
}

void str_err(const char* name)
{
    fprintf(stderr, "%s: %s\n", name, strerror(errno));
    exit(1);
}

int main(int argc,char** argv)
{
    if(argc < 3)
    {
        fprintf(stderr,"Usage: %s [dev] [port]\n",argv[0]);
        exit(1);
    }

    signal(SIGPIPE,sig_handler);
    camera_fd = open(argv[1],O_RDWR|O_NONBLOCK);


    install_dev();
    //cam_on();

    init_socket(atoi(argv[2]));
    start_listen(10);
    uninit_socket();
    //cam_off();
    uninstall_dev();

    return 0;
}


#endif
