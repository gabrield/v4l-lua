#ifndef CORE_H
#define CORE_H

#define WITH_V4L2_LIB 1		/* v4l library */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

struct buffer
{
	void *start;
	size_t length;
};

/* functions */

static int read_frame(void);
static int get_frame(void);
static int xioctl(int fd, int request, void *arg);
static void errno_exit(const char *s);
unsigned char *process_image(unsigned char *, int, int, int);
void stop_capturing(void);
void start_capturing(void);
void uninit_device(void);
void init_mmap(void);
void init_device();
int getwidth();
int getheight();
unsigned char *newframe();
void close_device(int dev);
int open_device(const char *dev);

#endif /*CORE_H*/
