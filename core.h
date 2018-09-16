/*
 Copyright (c) 2011 Gabriel Duarte <gabrield@impa.br>

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

#ifndef CORE_H
#define CORE_H

#define WITH_V4L2_LIB 1		/* v4l library */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <math.h>
#include <asm/types.h>		/* for videodev2.h */
#include <linux/videodev2.h>
#include <libv4lconvert.h>

struct buffer {
    void * start;
    size_t length;
};

struct device {
    int fd;
    char * path;
    int w, h;
    double timeout_secs;
    struct v4l2_format src_fmt, fmt;
    struct v4lconvert_data * v4lconvert_data;
    unsigned char * dst_buf;
    struct buffer * buffers;
    int n_buffers;
    void * errorfn_data;
    void (*errorfn)(void * errorfn_data, char * fmt, ...);
};

/* Refactored functions: on error return a negative value: */
int open_device(struct device * dev, const char * path,
    void * errorfn_data,
    void (*errorfn)(void * errorfn_data, char * fmt, ...));
int init_device(struct device * dev, int w, int h);
void close_device(struct device * dev);

int start_capturing(struct device * dev);
void set_timout(struct device * dev, double secs);
/* timeout on newframe can be detected via errorfn being reported the "timeout" string */
unsigned char * newframe(struct device * dev);

#endif /*CORE_H*/
/* vi: set et sw=4: */
