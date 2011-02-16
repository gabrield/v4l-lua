#include "core.h"

int fd =-1;
int w = 720, h = 480;
struct v4lconvert_data *v4lconvert_data;
struct v4l2_format src_fmt;    /* raw format */
struct v4l2_buffer buf;
unsigned char *dst_buf;
unsigned char *res_buf;
unsigned int mono = 0, inverse = 0;

struct v4l2_format fmt;
const char *dev_name;
int save_pnm(unsigned char *, int, int, int);
int io = V4L2_MEMORY_MMAP;
struct buffer *buffers;
int n_buffers;


int getwidth()
{
    return w;
}

int getheight()
{
    return h;
}

int xioctl(int fd, int request, void *arg)
{
    int r;

    do {
        r = ioctl(fd, request, arg);
    } while (r < 0 && EINTR == errno);
    return r;
}


 void errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    fprintf(stderr, "%s\n", v4lconvert_get_error_message(v4lconvert_data));
    exit(EXIT_FAILURE);
}
                                                  

unsigned char *newframe()
{
    get_frame();
    return process_image((unsigned char *)buffers[buf.index].start, buf.bytesused, w, h);
}


unsigned char *process_image(unsigned char *p, int len, int W, int H)
{

    if (WITH_V4L2_LIB) {
        if (v4lconvert_convert(v4lconvert_data,
                       &src_fmt,
                       &fmt,
                       p, len,
                       dst_buf,
                       fmt.fmt.pix.sizeimage) < 0) {
            if (errno != EAGAIN)
                errno_exit("v4l_convert");
            return;
        }
        p = dst_buf;
        //p = (unsigned char *)realloc(dst_buf,fmt.fmt.pix.sizeimage);
        len = fmt.fmt.pix.sizeimage;
    }
    return dst_buf;
}

 int read_frame(void)
{
/*    printf("%s\n", dev_name);*/
    memset(&(buf), 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if(xioctl(fd, VIDIOC_DQBUF, &buf) < 0)
    {
        switch (errno)
        {
        case EAGAIN:
            return 0;
            break;
        case EIO:
        /* Could ignore EIO, see spec. */
        /* fall through */

        default:
            /*errno_exit("VIDIOC_DQBUF");*/
            perror("VIDIOC_DQBUF");
        }
    }
        
       assert((unsigned char)buf.index < n_buffers);
    /*process_image((unsigned char*)buffers[buf.index].start, buf.bytesused, w, h);*/

    if (xioctl(fd, VIDIOC_QBUF, &buf) < 0)
        perror("VIDIOC_QBUF");
        /*errno_exit("VIDIOC_QBUF");*/

    return 1;
}

 int get_frame(void)
{
    fd_set fds;
    struct timeval tv;
    int r;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    /* Timeout. */
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    r = select(fd + 1, &fds, NULL, NULL, &tv);
    if (r < 0) {
        if (EINTR == errno)
            return 0;

        perror("select");
        //errno_exit("select");
    }

    if (0 == r) {
        perror("select timeout");
        //exit(EXIT_FAILURE);
    }
    read_frame();
    return 0;
}

void mainloop(void)
{
    int count;

    count = 1000;
    while (--count >= 0)
    {
            get_frame();
    }
}

 void stop_capturing(void)
{
    switch (io) {
    case V4L2_MEMORY_MMAP:
        /*Nothing to do */
        ("Stop Capturing...\n");
    }
}

void start_capturing(void)
{
    int i;
    enum v4l2_buf_type type;
    struct v4l2_buffer buf;

    switch (io) {
    case V4L2_MEMORY_MMAP:
        printf("mmap method\n");
        for (i = 0; i < n_buffers; ++i) {

            memset(&(buf), 0, sizeof(buf));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            if (xioctl(fd, VIDIOC_QBUF, &buf) < 0)
                perror("VIDIOC_QBUF");
        }

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (xioctl(fd, VIDIOC_STREAMON, &type) < 0)
            perror("VIDIOC_STREAMON");
            return;
          break;
    }
}

 void uninit_device(void)
{
    int i;

    switch (io) {
    case V4L2_MEMORY_MMAP:
        for (i = 0; i < n_buffers; ++i)
            if (-1 == munmap(buffers[i].start, buffers[i].length))
                errno_exit("munmap");
        break;
    }
    
/*    if(dst_buf != NULL)
        free(dst_buf);*/
              
    if(buffers != NULL)
        free(buffers);
}



 void init_mmap(void)
{
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    memset(&(req), 0, sizeof(req));
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;


    if(xioctl(fd, VIDIOC_REQBUFS, &req) < 0)
    {
        if(EINVAL == errno)
        {
            fprintf(stderr, "%s does not support memory mapping\n", dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if(req.count < 2)
    {
        fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
        exit(EXIT_FAILURE);
    }


    buffers = (struct buffer*) calloc(req.count, sizeof(struct buffer));

    if (!buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < (unsigned char)req.count; ++n_buffers) {

        memset(&(buf), 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
            errno_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL /* start anywhere */ ,
                        buf.length,
                        PROT_READ | PROT_WRITE
                        /* required */ ,
                        MAP_SHARED
                        /* recommended */ ,
                        fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start)
            errno_exit("mmap");
    }
}

void init_device()
{
    struct v4l2_capability cap;
    int ret;
    int sizeimage;

    if (xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n", dev_name);
            /*exit(EXIT_FAILURE);*/
            perror("EXIT_FAILURE");
            return;
        } else {
            /*errno_exit("VIDIOC_QUERYCAP");*/
            perror("VIDIOC_QUERYCAP");
            return ;
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\n", dev_name);
        /*exit(EXIT_FAILURE);*/
        perror("EXIT_FAILURE");
        return;
    }

    memset(&(fmt), 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = w;
    fmt.fmt.pix.height = h;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (WITH_V4L2_LIB) {
        v4lconvert_data = v4lconvert_create(fd);
        if (v4lconvert_data == NULL)
        {
            perror("v4lconvert_create");
            return;
        }
        
            /*errno_exit("v4lconvert_create");*/
        if (v4lconvert_try_format(v4lconvert_data, &fmt, &src_fmt) != 0)
        {
        /*errno_exit("v4lconvert_try_format");*/
            perror("v4lconvert_try_format");
            return;
        }
        ret = xioctl(fd, VIDIOC_S_FMT, &src_fmt);
        sizeimage = src_fmt.fmt.pix.sizeimage;
        dst_buf = (unsigned char *)malloc(fmt.fmt.pix.sizeimage);
        printf("raw pixfmt: %c%c%c%c %dx%d\n",
               src_fmt.fmt.pix.pixelformat & 0xff,
               (src_fmt.fmt.pix.pixelformat >> 8) & 0xff,
               (src_fmt.fmt.pix.pixelformat >> 16) & 0xff,
               (src_fmt.fmt.pix.pixelformat >> 24) & 0xff,
               src_fmt.fmt.pix.width, src_fmt.fmt.pix.height);
    } else {
        ret = xioctl(fd, VIDIOC_S_FMT, &fmt);
        sizeimage = fmt.fmt.pix.sizeimage;
    }
    if (ret < 0)
    {
        perror("VIDIOC_S_FMT");
/*        errno_exit("VIDIOC_S_FMT");*/
        return;
    }
    /* Note VIDIOC_S_FMT may change width and height. */
    
    printf("pixfmt: %c%c%c%c %dx%d\n",
           fmt.fmt.pix.pixelformat & 0xff,
           (fmt.fmt.pix.pixelformat >> 8) & 0xff,
           (fmt.fmt.pix.pixelformat >> 16) & 0xff,
           (fmt.fmt.pix.pixelformat >> 24) & 0xff,
           fmt.fmt.pix.width, fmt.fmt.pix.height);

    w = fmt.fmt.pix.width;
    h = fmt.fmt.pix.height;


    printf("V4L2_MEMORY_MMAP\n");
    init_mmap();
    
}

void close_device(int dev)
{
    printf("Closing Device\n");
    close(dev);
}

int open_device(const char *dev)
{
    struct stat st;

    if (stat(dev, &st) < 0) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n",
            dev, errno, strerror(errno));
        /*exit(EXIT_FAILURE);*/
    }

    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is no device\n", dev);
/*        exit(EXIT_FAILURE);*/
    }

    fd = open(dev, O_RDWR /* required */  | O_NONBLOCK, 0);
    if (fd < 0) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n",
            dev, errno, strerror(errno));
/*        exit(EXIT_FAILURE);*/
    }
    return fd;
}


