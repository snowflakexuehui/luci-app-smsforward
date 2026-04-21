/*
 * Copyright 2012 - 2014 Thomas Buck <xythobuz@xythobuz.de>
 */

#include "serial.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#ifndef XON
#define XON 0x11
#endif

#ifndef XOFF
#define XOFF 0x13
#endif

#ifndef TIMEOUT
#define TIMEOUT 2
#endif

int serialOpen(const char *port, unsigned int baud) {
    struct termios options;

    int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd == -1) {
        fprintf(stderr, "Couldn't open port \"%s\": %s\n", port, strerror(errno));
        return -1;
    }

    tcgetattr(fd, &options);

    // 设置原始模式，禁用行处理/回显/输出处理
    cfmakeraw(&options);

    // Set Baudrate
    switch (baud) {
        case 9600:
            cfsetispeed(&options, B9600);
            cfsetospeed(&options, B9600);
            break;
        case 19200:
            cfsetispeed(&options, B19200);
            cfsetospeed(&options, B19200);
            break;
        case 38400:
            cfsetispeed(&options, B38400);
            cfsetospeed(&options, B38400);
            break;
        // case 76800:
        //   cfsetispeed(&options, B76800);
        //  cfsetospeed(&options, B76800);
        //  break;
        case 115200:
            cfsetispeed(&options, B115200);
            cfsetospeed(&options, B115200);
            break;
        default:
            fprintf(stderr, "Warning: Baudrate not supported!\n");
            serialClose(fd);
            return -1;
    }

    // 控制模式：8N1 且忽略调制解调线状态
    options.c_cflag |= CS8 | CREAD | CLOCAL;
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CRTSCTS; // 禁用硬件流控

    // 输入/输出：关闭软件流控与特殊处理
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;

    // 读超时：VMIN=0, VTIME=5 (0.5秒单位1/10秒)
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 5;
#ifdef XONXOFF
    options.c_cc[VSTOP] = XOFF;
    options.c_cc[VSTART] = XON;
#endif

    tcsetattr(fd, TCSANOW, &options);

    tcflush(fd, TCIOFLUSH);

    return fd;
}

void serialClose(int fd) {
    tcflush(fd, TCIOFLUSH);
    close(fd);
}

int serialHasChar(int fd) {
    struct pollfd fds;
    fds.fd = fd;
    fds.events = (POLLIN | POLLPRI);  // Data may be read
    if (poll(&fds, 1, 0) > 0) {
        return 1;
    } else {
        return 0;
    }
}

void serialWaitUntilSent(int fd) {
    while (tcdrain(fd) == -1) {
        fprintf(stderr, "Could not drain data: %s\n", strerror(errno));
    }
}

unsigned int serialWriteRaw(int fd, const char *d, int len) {
    unsigned int processed = 0;
    time_t start = time(NULL);

    while ((processed < len) && (difftime(time(NULL), start) < TIMEOUT)) {
        int t = write(fd, (d + processed), (len - processed));
        if (t == -1) {
            fprintf(stderr, "Error while writing: %s\n", strerror(errno));
            return processed;
        } else {
            processed += t;
        }
    }

    return processed;
}

unsigned int serialReadRaw(int fd, char *d, int len) {
    unsigned int processed = 0;
    time_t start = time(NULL);

    while ((processed < len) && (difftime(time(NULL), start) < TIMEOUT)) {
        int t = read(fd, (d + processed), (len - processed));
        if (t == -1) {
            // fprintf(stderr, "Error while reading: %s\n", strerror(errno));
            return processed;
        } else {
            processed += t;
        }
    }

    return processed;
}

void serialWriteChar(int fd, char c) { while (serialWriteRaw(fd, &c, 1) != 1); }

void serialReadChar(int fd, char *c) {
    while (serialReadRaw(fd, c, 1) != 1);
#ifdef XONXOFF
    if (*c == XON) {
        if (tcflow(fd, TCOON) == -1) {
            fprintf(stderr, "Could not restart flow: %s\n", strerror(errno));
        }
        serialReadChar(fd, c);
    } else if (*c == XOFF) {
        if (tcflow(fd, TCOOFF) == -1) {
            fprintf(stderr, "Could not stop flow: %s\n", strerror(errno));
        }
        serialReadChar(fd, c);
    }
#endif
}

void serialWriteString(int fd, const char *s) {
    while (*s) serialWriteChar(fd, *(s++));
}

char **getSerialPorts(void) {
    DIR *dir;
    struct dirent *ent;

    int size = 0;
    dir = opendir("/dev/");
    while ((ent = readdir(dir)) != NULL) {
#ifdef SEARCH
        if (strstr(ent->d_name, SEARCH) != NULL)
#endif
            size++;
    }
    closedir(dir);

    char **files = (char **)malloc((size + 1) * sizeof(char *));

    int i = 0;
    dir = opendir("/dev/");
    while (((ent = readdir(dir)) != NULL) && (i < size)) {
#ifdef SEARCH
        if (strstr(ent->d_name, SEARCH) != NULL) {
#endif

            int tmp = strlen(ent->d_name) + 6;
            files[i] = (char *)malloc(tmp * sizeof(char));
            strcpy(files[i], "/dev/");
            strcpy(files[i] + 5, ent->d_name);
            files[i][tmp - 1] = '\0';

#ifdef TRY_TO_OPEN_PORTS
            int fdtmp = serialOpen(files[i], 9600);
            if (fdtmp != -1) {
                serialClose(fdtmp);
#endif

                i++;

#ifdef TRY_TO_OPEN_PORTS
            } else {
                free(files[i]);
            }
#endif

#ifdef SEARCH
        }
#endif
    }
    closedir(dir);
    files[i] = NULL;
    return files;
}
