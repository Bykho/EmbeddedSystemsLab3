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
#define SLEEP_TIME     5  // 500ms delay between updates


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

    // Main loop
    while (1) {
        // Update theta
        if (theta >= 175.0f) clockWise = false;
        else if (theta <= 5.0f) clockWise = true;
        theta += (clockWise ? +1.0f : -1.0f);

        // Convert and clamp to int
        angle = (int)roundf(theta);

        // Update chirp state at specific angles
        chirp = 1;
        uint32_t sum_status = 0;
        int valid_readings = 0;
        
        for (int i = 0; i < 1000; i++) {
            // Pack 32-bit config: [31:16]=time, [15:0]=chirp
            uint16_t timeout = 65535;  // Max 16-bit value
            uint32_t cfg = ((timeout & 0xFFFF) << 16) | (chirp & 0x1);
            //printf("Writing config: timeout=0x%04x, chirp=%d, cfg=0x%08x\n", timeout, chirp, cfg);
            if (ioctl(us_fd, US_WRITE_CONFIG, &cfg) < 0) {
                perror("US_WRITE_CONFIG failed");
                break;
            }

            // Every 2°, read and print status from main thread
            if (angle % 1 == 0) {
                uint32_t status;
                if (ioctl(us_fd, US_READ_STATUS, &status) < 0) {
                    perror("US_READ_STATUS failed");
                    break;
                }
                //printf("Echo status @ %3d° = 0x%08x, chirp = %d\n", angle, status, chirp);
                
                // Add to average if not timeout value
                if (status != 0x80000003) {
                    sum_status += status;
                    valid_readings++;
                }
            }
            
            usleep(1);
        }

        int final_distance = (valid_readings > 0) ? sum_status : 0;
        printf("Final max distance: %d\n", final_distance);

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

        usleep(SLEEP_TIME);
    }

    close(us_fd);
    close(vga_ball_fd);
    return 0;
}


