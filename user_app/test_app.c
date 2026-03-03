#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "../driver/embedded_char_driver.h"

int main()
{
    int fd;
    char buffer[1024];
    int irq_count;

    fd = open("/dev/embedded_char", O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open device");
        return -1;
    }

    strcpy(buffer, "Hello from user space!");
    write(fd, buffer, sizeof(buffer));

    read(fd, buffer, sizeof(buffer));
    printf("Read from device: %s\n", buffer);

    ioctl(fd, IOCTL_GET_COUNTER, &irq_count);
    printf("Interrupt count: %d\n", irq_count);

    ioctl(fd, IOCTL_RESET_COUNTER);

    close(fd);
    return 0;
}
