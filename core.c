/*
   Copyright (c) 2011 Gabriel Duarte <confusosk8@gmail.com>

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
   */

#include "core.h"

static const int N_BUFFERS = 4;

static int xioctl(int fd, int request, void * arg) {
    int r;

    do {
        r = ioctl(fd, request, arg);
    } while (r < 0 && EINTR == errno);
    return r;
}

static int process_image(struct device * dev, unsigned char * p, int len) {

    if (v4lconvert_convert(dev->v4lconvert_data,
                &dev->src_fmt,
                &dev->fmt,
                p, len,
                dev->dst_buf,
                dev->fmt.fmt.pix.sizeimage) < 0) {
        if(errno != EAGAIN) {
            dev->errorfn(dev->errorfn_data, "v4l_convert: %s", v4lconvert_get_error_message(dev->v4lconvert_data));
            return -1;
        }
    }
    return 0;
}

static int read_frame(struct device * dev, struct v4l2_buffer * buf) {
    int res;
    memset(buf, 0, sizeof *buf);
    buf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf->memory = V4L2_MEMORY_MMAP;

    if (xioctl(dev->fd, VIDIOC_DQBUF, buf) < 0) {
        switch (errno) {
            case EAGAIN:
                return 0;
                break;
            case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
            default:
                dev->errorfn(dev->errorfn_data, "VIDIOC_DQBUF: %s", strerror(errno));
                return -1;
        }
    }

    assert(buf->index < dev->n_buffers);
    res = process_image(dev, dev->buffers[buf->index].start, buf->bytesused);

    if (xioctl(dev->fd, VIDIOC_QBUF, buf) < 0) {
        dev->errorfn(dev->errorfn_data, "VIDIOC_QBUF: %s", strerror(errno));
        return -1;
    }

    return res;
}

static int get_frame(struct device * dev) {
    fd_set fds;
    struct timeval tv;
    int r;

    FD_ZERO(&fds);
    FD_SET(dev->fd, &fds);

    /* Timeout. */
    tv.tv_sec = dev->timeout_secs;
    tv.tv_usec = fmod(dev->timeout_secs, 1.0) * 1e6;

    do {
        r = select(dev->fd + 1, &fds, NULL, NULL, &tv);
    } while (r < 0 && errno == EINTR);

    if (r < 0) {
        dev->errorfn(dev->errorfn_data, "select: %s", strerror(errno));
        return -1;
    } else if (r == 0) {
        dev->errorfn(dev->errorfn_data, "timeout");
        return -1;
    }

    return 0;
}

unsigned char * newframe(struct device * dev) {
    struct v4l2_buffer buf;

    if (get_frame(dev) < 0)
        return NULL;
    if (read_frame(dev, &buf) < 0)
        return NULL;
    return dev->dst_buf;
}

void set_timout(struct device * dev, double secs) {
    dev->timeout_secs = secs;
}

int start_capturing(struct device * dev) {
    int i;
    enum v4l2_buf_type type;
    struct v4l2_buffer buf;

    /*printf("mmap method\n");*/

    for (i = 0; i < dev->n_buffers; i++) {
        memset(&buf, 0, sizeof buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(dev->fd, VIDIOC_QBUF, &buf) < 0) {
            dev->errorfn(dev->errorfn_data, "VIDIOC_QBUF: %s", strerror(errno));
            return -1;
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (xioctl(dev->fd, VIDIOC_STREAMON, &type) < 0) {
        dev->errorfn(dev->errorfn_data, "VIDIOC_STREAMON: %s", strerror(errno));
        return -1;
    }
    return 0;
}

static int init_mmap(struct device * dev) {
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    int i;

    memset(&(req), 0, sizeof(req));
    req.count = N_BUFFERS;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    /* call to VIDIOC_REQBUFS may change req.count */
    if (xioctl(dev->fd, VIDIOC_REQBUFS, &req) < 0) {
        if (EINVAL == errno) {
            dev->errorfn(dev->errorfn_data, "%s does not support memory mapping", dev->path);
        } else {
            dev->errorfn(dev->errorfn_data, "VIDIOC_REQBUFS: %s", strerror(errno));
        }
        return -1;
    }

    if (req.count < 2) {
        dev->errorfn(dev->errorfn_data, "Insufficient buffer memory on %s", dev->path);
        return -1;
    }

    dev->buffers = (struct buffer *) calloc(req.count, sizeof (struct buffer));
    dev->n_buffers = req.count;

    if (!dev->buffers) {
        dev->errorfn(dev->errorfn_data, "Out of memory: %s", strerror(errno));
        return -1;
    }

    for (i = 0; i < dev->n_buffers; i++) {
        memset(&buf, 0, sizeof buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl(dev->fd, VIDIOC_QUERYBUF, &buf) < 0) {
            dev->errorfn(dev->errorfn_data, "VIDIOC_QUERYBUF: %s", strerror(errno));
            return -1;
        }

        dev->buffers[i].length = buf.length;
        dev->buffers[i].start = mmap(NULL, /* start anywhere */
                buf.length,
                PROT_READ | PROT_WRITE, /* required */
                MAP_SHARED, /* recommended */
                dev->fd, buf.m.offset);

        if (MAP_FAILED == dev->buffers[i].start) {
            dev->buffers[i].start = NULL;
            dev->errorfn(dev->errorfn_data, "mmap: %s", strerror(errno));
            return -1;
        }
    }
    return 0;
}

int init_device(struct device * dev, int w, int h) {
    struct v4l2_capability cap;
    int ret;

    if (xioctl(dev->fd, VIDIOC_QUERYCAP, &cap) < 0) {
        if (EINVAL == errno) {
            dev->errorfn(dev->errorfn_data, "%s is no V4L2 device", dev->path);
            return -1;
        } else {
            dev->errorfn(dev->errorfn_data, "VIDIOC_QUERYCAP: %s", strerror(errno));
            return -1;
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        dev->errorfn(dev->errorfn_data, "%s is no video capture device", dev->path);
        return -1;
    }

    memset(&dev->fmt, 0, sizeof dev->fmt);
    dev->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    dev->fmt.fmt.pix.width = w;
    dev->fmt.fmt.pix.height = h;
    dev->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    dev->fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    dev->v4lconvert_data = v4lconvert_create(dev->fd);

    if (!dev->v4lconvert_data) {
        dev->errorfn(dev->errorfn_data, "v4lconvert_create failure: %s", strerror(errno));
        return -1;
    }

    if (v4lconvert_try_format(dev->v4lconvert_data, &dev->fmt, &dev->src_fmt) != 0) {
        /*errno_exit("v4lconvert_try_format");*/
        dev->errorfn(dev->errorfn_data, "v4lconvert_try_format: %s", v4lconvert_get_error_message(dev->v4lconvert_data));
        return -1;
    }

    dev->w = dev->fmt.fmt.pix.width;
    dev->h = dev->fmt.fmt.pix.height;

    ret = xioctl(dev->fd, VIDIOC_S_FMT, &dev->src_fmt);
    dev->dst_buf = (unsigned char *)malloc(dev->fmt.fmt.pix.sizeimage);

#ifdef DEBUG
    printf("raw pixfmt: %c%c%c%c %dx%d\n",
            dev->src_fmt.fmt.pix.pixelformat & 0xff,
            (dev->src_fmt.fmt.pix.pixelformat >> 8) & 0xff,
            (dev->src_fmt.fmt.pix.pixelformat >> 16) & 0xff,
            (dev->src_fmt.fmt.pix.pixelformat >> 24) & 0xff,
            dev->src_fmt.fmt.pix.width, dev->src_fmt.fmt.pix.height);
#endif

    if (ret < 0) {
        dev->errorfn(dev->errorfn_data, "VIDIOC_S_FMT");
        return -1;
    }

#ifdef DEBUG
    printf("pixfmt: %c%c%c%c %dx%d\n",
            dev->fmt.fmt.pix.pixelformat & 0xff,
            (dev->fmt.fmt.pix.pixelformat >> 8) & 0xff,
            (dev->fmt.fmt.pix.pixelformat >> 16) & 0xff,
            (dev->fmt.fmt.pix.pixelformat >> 24) & 0xff,
            dev->fmt.fmt.pix.width, dev->fmt.fmt.pix.height);

    /* Note VIDIOC_S_FMT may change width and height. */
#endif

    w = dev->fmt.fmt.pix.width;
    h = dev->fmt.fmt.pix.height;

    return init_mmap(dev);
}

void close_device(struct device * dev) {
    int i;

    if (dev->path) {
        free(dev->path);
        dev->path = NULL;
    }
    if (dev->v4lconvert_data) {
        v4lconvert_destroy(dev->v4lconvert_data);
        dev->v4lconvert_data = NULL;
    }
    if (dev->fd >= 0) {
        close(dev->fd);
        dev->fd = -1;
    }
    if (dev->dst_buf) {
        free(dev->dst_buf);
        dev->dst_buf = NULL;
    }
    if (dev->buffers) {
        for (i = 0; i < dev->n_buffers; i++)
            if (dev->buffers[i].start)
                munmap(dev->buffers[i].start, dev->buffers[i].length);
        free(dev->buffers);
        dev->buffers = NULL;
    }
}

static void default_errorfn(void * errorfn_data, char * fmt, ...) {
    va_list args;
    (void)errorfn_data;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    putc('\n', stderr);
}

int open_device(struct device * dev, const char * path,
        void * errorfn_data,
        void (*errorfn)(void * errorfn_data, char * fmt, ...)) {
    struct stat st;

    dev->fd = -1;
    dev->path = NULL;
    dev->timeout_secs = 2;
    dev->v4lconvert_data = NULL;
    dev->dst_buf = NULL;
    dev->buffers = NULL;
    dev->n_buffers = 0;
    dev->errorfn_data = errorfn_data;
    dev->errorfn = errorfn ? errorfn : &default_errorfn;

    if (stat(path, &st) < 0) {
        dev->errorfn(dev->errorfn_data, "Cannot identify '%s': %d, %s", path, errno, strerror(errno));
        return -1;
    }

    if (!S_ISCHR(st.st_mode)) {
        dev->errorfn(dev->errorfn_data, "%s is no device", path);
        return -1;
    }

    dev->fd = open(path, O_RDWR /* required */  | O_NONBLOCK, 0);

    if (dev->fd < 0) {
        dev->errorfn(dev->errorfn_data, "Cannot open '%s': %d, %s", path, errno, strerror(errno));
        return -1;
    }

    dev->path = strcpy(malloc(strlen(path) + 1), path);

    return dev->fd;
}

/* vi: set et sw=4: */
