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
    
    printf("Testing ultrasonic sensor state transitions...\n");
    
    // Read initial state
    if (ioctl(fd, US_READ_STATUS, &status) < 0) {
        perror("ioctl read");
        close(fd);
        return 1;
    }
    printf("Initial status: 0x%08x\n\n", status);
    
    // Rapid state transitions
    for (int i = 0; i < 5; i++) {
        // Set chirp=1
        uint32_t cfg_on = (65535 << 16) | 1;
        printf("Cycle %d: Setting chirp=1 (0x%08x)\n", i, cfg_on);
        if (ioctl(fd, US_WRITE_CONFIG, &cfg_on) < 0) {
            perror("ioctl write");
            break;
        }
        
        // Short delay
        usleep(2000000);  // 2s
        

        // Read status immediately
        if (ioctl(fd, US_READ_STATUS, &status) < 0) {
            perror("ioctl read");
            break;
        }
        printf("  Status after chirp=1: 0x%08x\n", status);


        // Short delay
        usleep(2000000);  // 2s

        // Set chirp=0
        uint32_t cfg_off = (65535 << 16) | 0;
        printf("Cycle %d: Setting chirp=0 (0x%08x)\n", i, cfg_off);
        if (ioctl(fd, US_WRITE_CONFIG, &cfg_off) < 0) {
            perror("ioctl write");
            break;
        }
        
        // Short delay
        usleep(2000000);  // 2s

        // Read status immediately
        if (ioctl(fd, US_READ_STATUS, &status) < 0) {
            perror("ioctl read");
            break;
        }
        printf("  Status after chirp=0: 0x%08x\n\n", status);
        
        // Short delay between cycles
        usleep(100000);  // 1s
    }
    
    // Try different timeout values
    printf("\nTesting different timeout values with chirp=1...\n");

    // Reset to idle
    uint32_t cfg_idle = 0;
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