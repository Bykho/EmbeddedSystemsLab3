#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
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
    
    printf("Testing ultrasonic sensor...\n");
    
    // Test 1: Read initial status
    if (ioctl(fd, US_READ_STATUS, &status) < 0) {
        perror("ioctl read");
        close(fd);
        return 1;
    }
    printf("Initial status: 0x%08x\n", status);
    
    // Test 2: Write chirp=0
    uint32_t cfg1 = (65535 << 16) | 0;
    printf("Writing config: chirp=0, timeout=65535 (0x%08x)\n", cfg1);
    if (ioctl(fd, US_WRITE_CONFIG, &cfg1) < 0) {
        perror("ioctl write");
        close(fd);
        return 1;
    }
    
    // Read status after chirp=0
    if (ioctl(fd, US_READ_STATUS, &status) < 0) {
        perror("ioctl read");
        close(fd);
        return 1;
    }
    printf("Status after chirp=0: 0x%08x\n", status);
    
    // Test 3: Write chirp=1
    uint32_t cfg2 = (65535 << 16) | 1;
    printf("Writing config: chirp=1, timeout=65535 (0x%08x)\n", cfg2);
    if (ioctl(fd, US_WRITE_CONFIG, &cfg2) < 0) {
        perror("ioctl write");
        close(fd);
        return 1;
    }
    
    // Read status multiple times after chirp=1
    for (int i = 0; i < 5; i++) {
        if (ioctl(fd, US_READ_STATUS, &status) < 0) {
            perror("ioctl read");
            break;
        }
        printf("Status %d after chirp=1: 0x%08x\n", i, status);
        usleep(100000);  // 100ms delay between reads
    }
    
    // Test 4: Write chirp=0 again
    printf("Writing config: chirp=0, timeout=65535 (0x%08x)\n", cfg1);
    if (ioctl(fd, US_WRITE_CONFIG, &cfg1) < 0) {
        perror("ioctl write");
        close(fd);
        return 1;
    }
    
    // Read status after changing back to chirp=0
    if (ioctl(fd, US_READ_STATUS, &status) < 0) {
        perror("ioctl read");
        close(fd);
        return 1;
    }
    printf("Final status after chirp=0: 0x%08x\n", status);
    
    close(fd);
    return 0;
}