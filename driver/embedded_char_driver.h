#ifndef EMBEDDED_CHAR_DRIVER_H
#define EMBEDDED_CHAR_DRIVER_H

#include <linux/ioctl.h>

#define DEVICE_NAME "embedded_char"
#define CLASS_NAME  "embedded_class"

#define BUFFER_SIZE 1024

/* IOCTL Commands */
#define IOCTL_RESET_COUNTER _IO('a', 'a')
#define IOCTL_GET_COUNTER   _IOR('a', 'b', int *)

#endif
