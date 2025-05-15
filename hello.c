#include <stdio.h>
#include <stdint.h>        // for uint32_t
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <math.h>
#include <stdbool.h>
#include "vga_ball.h"
#include "ultrasonic_sensor.h"

#define SCREEN_WIDTH   640
#define SCREEN_HEIGHT  480
#define VGA_BUFFER_HEIGHT 256 // Max lines for the VGA driver buffer
#define SLEEP_TIME     500000  // 500ms delay between updates


int main(void) {
    int  vga_ball_fd, us_fd;
    vga_ball_line_t   vla_line;
    const char       *vga_dev = "/dev/vga_ball";
    float             theta  = 1.0f;
    bool              clockWise = true;
    uint16_t          chirp = 0;            // current chirp bit
    int               angle;
    int               thread_running = 0;

    // Open VGA device
    if ((vga_ball_fd = open(vga_dev, O_RDWR)) < 0) {
        perror("open vga_ball");
        return 1;
    }

    // Open ultrasonic device
    if ((us_fd = open("/dev/ultrasonic_sensor", O_RDWR)) < 0) {
        perror("open ultrasonic_sensor");
        close(vga_ball_fd);
        return 1;
    }

    // Pre-fill line matrix
    int LineMatrix[VGA_BUFFER_HEIGHT][2];
    for (int y = 0; y < VGA_BUFFER_HEIGHT; y++) {
        LineMatrix[y][0] = SCREEN_WIDTH/2;
        LineMatrix[y][1] = SCREEN_WIDTH/2;
    }

    int TIMEOUT_LIMIT = 500;
    uint32_t status = 0;
    int counter = 0;
    // Main loop
    while (1) {
        angle = (int)roundf(theta);
        // Update theta
        if (counter == 1000000) {
            if (theta >= 175.0f) clockWise = false;
            else if (theta <= 5.0f) clockWise = true;
            theta += (clockWise ? +1.0f : -1.0f);
            counter = 0;
        }
        int distance = 0;


        if (ioctl(us_fd, US_READ_STATUS, &status) < 0) {
            perror("US_READ_STATUS failed");
            break;
        }
        
        if (status == 0){
            chirp = 1;
        } else if (status > 3) {
            if (status < TIMEOUT_LIMIT) {
                distance = status;
            } 
            chirp = 0;
        }
        if (status != 3 || status != 0 || status != 1 || status != 2) {
            printf("Echo status @ %3d° = 0x%08x, chirp = %d\n", angle, status, chirp);
        }
        uint16_t timeout = TIMEOUT_LIMIT;  // Max 16-bit value
        uint32_t cfg = ((timeout & 0xFFFF) << 16) | (chirp & 0x1);
        //printf("Writing config: timeout=0x%04x, chirp=%d, cfg=0x%08x\n", timeout, chirp, cfg);
             if (ioctl(us_fd, US_WRITE_CONFIG, &cfg) < 0) {
                 perror("US_WRITE_CONFIG failed");
                 break;
             }
        int AngleDistanceFrom90 = fabs(90 - angle) / 30;

        // Compute line geometry
        int num = (int)(VGA_BUFFER_HEIGHT * sinf(theta * (float)M_PI / 180.0f));

        if (num < 0) num = 0;
        if (num > SCREEN_HEIGHT) num = SCREEN_HEIGHT;

        for (int y = 0; y < VGA_BUFFER_HEIGHT; y++) {
            if (y < num) {
                int vx = (num > 0)
                    ? (int)(((float)VGA_BUFFER_HEIGHT / num) * y)
                    : 0;
                int x0 = SCREEN_WIDTH/2 + (int)(cosf(theta * (float)M_PI / 180.0f) * vx) - (2 + AngleDistanceFrom90);
                int x1 = SCREEN_WIDTH/2 + (int)(cosf(theta * (float)M_PI / 180.0f) * vx) + (2 + AngleDistanceFrom90);

                // Clamp
                if (x0 < 0) x0 = 0;
                if (x1 >= SCREEN_WIDTH) x1 = SCREEN_WIDTH - 1;
                LineMatrix[y][0] = x0;
                LineMatrix[y][1] = x1;
            } else {
                LineMatrix[y][0] = -1;
                LineMatrix[y][1] = -1;
            }
        }

        // Draw to VGA
        memcpy(vla_line.LineMatrix, LineMatrix, sizeof(LineMatrix));
        if (ioctl(vga_ball_fd, VGA_BALL_WRITE_LINE, &vla_line) < 0) {
            perror("VGA_BALL_WRITE_LINE failed");
            break;
        }

        counter++;
    }

    close(us_fd);
    close(vga_ball_fd);
    return 0;   
}


