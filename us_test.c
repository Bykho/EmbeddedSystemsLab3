#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "ultrasonic_sensor.h"

int main() {
    int fd;
    uint32_t status;
    
    // Open device
    if ((fd = open("/dev/ultrasonic_sensor", O_RDWR)) < 0) {
        perror("open");
        return 1;
    }
    
    printf("Testing ultrasonic sensor timeout feature...\n");
    
    // Test 1: Set timeout to a very small value
    uint32_t cfg_short_timeout = (10 << 16) | 1;  // Timeout=10, chirp=1
    printf("Writing config: chirp=1, short timeout=10 (0x%08x)\n", cfg_short_timeout);
    if (ioctl(fd, US_WRITE_CONFIG, &cfg_short_timeout) < 0) {
        perror("ioctl write");
        close(fd);
        return 1;
    }
    
    // Read status several times to see if timeout happens
    printf("Reading status with short timeout...\n");
    for (int i = 0; i < 5; i++) {
        if (ioctl(fd, US_READ_STATUS, &status) < 0) {
            perror("ioctl read");
            break;
        }
        printf("Status %d: 0x%08x\n", i, status);
        usleep(50000);  // 50ms delay
    }
    
    // Test 2: Set timeout to a large value
    uint32_t cfg_long_timeout = (65535 << 16) | 1;  // Timeout=65535, chirp=1
    printf("\nWriting config: chirp=1, long timeout=65535 (0x%08x)\n", cfg_long_timeout);
    if (ioctl(fd, US_WRITE_CONFIG, &cfg_long_timeout) < 0) {
        perror("ioctl write");
        close(fd);
        return 1;
    }
    
    // Read status several times with long timeout
    printf("Reading status with long timeout...\n");
    for (int i = 0; i < 5; i++) {
        if (ioctl(fd, US_READ_STATUS, &status) < 0) {
            perror("ioctl read");
            break;
        }
        printf("Status %d: 0x%08x\n", i, status);
        usleep(50000);  // 50ms delay
    }
    
    // Test 3: Reset to idle
    uint32_t cfg_idle = 0;  // Timeout=0, chirp=0
    printf("\nResetting to idle (0x%08x)\n", cfg_idle);
    if (ioctl(fd, US_WRITE_CONFIG, &cfg_idle) < 0) {
        perror("ioctl write");
        close(fd);
        return 1;
    }
    
    // Final status check
    if (ioctl(fd, US_READ_STATUS, &status) < 0) {
        perror("ioctl read");
        close(fd);
        return 1;
    }
    printf("Final status: 0x%08x\n", status);
    
    close(fd);
    return 0;
}