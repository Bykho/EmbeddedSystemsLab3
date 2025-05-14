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
#define SLEEP_TIME     50000  // 50ms delay between updates

int main(void) {
    int  vga_ball_fd, us_fd;
    vga_ball_line_t   vla_line;
    const char       *vga_dev = "/dev/vga_ball";
    float             theta  = 1.0f;
    bool              clockWise = true;
    uint16_t          chirp = 0;            // current chirp bit
    int               angle;

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
        if (theta >= 178.0f) clockWise = false;
        else if (theta <= 1.0f) clockWise = true;
        theta += (clockWise ? +1.0f : -1.0f);

        // Convert and clamp to int
        angle = (int)roundf(theta);

        // Update chirp state at 2° and 177°
        if (angle == 2) {
            chirp = 1;
        } else if (angle == 177) {
            chirp = 0;
        }

        // Pack 32-bit config: [31:16]=angle, [15:0]=chirp
        uint32_t cfg = ((uint32_t)angle << 16) | (chirp & 0xFFFF);
        if (ioctl(us_fd, US_WRITE_CONFIG, &cfg) < 0) {
            perror("US_WRITE_CONFIG failed");
            break;
        }

        // Every 10°, read and print status
        if (angle % 10 == 0) {
            char status[33];
            if (ioctl(us_fd, US_READ_STATUS, &status) < 0) {
                perror("US_READ_STATUS failed");
                break;
            }
            status[32] = '\0';
            printf("Echo status @ %3d° = 0x%s, chirp = %d\n", angle, status, chirp);
        }

        // Compute line geometry
        int num = (int)(VGA_BUFFER_HEIGHT * sinf(theta * (float)M_PI / 180.0f));
        if (num < 0) num = 0;
        if (num > SCREEN_HEIGHT) num = SCREEN_HEIGHT;

        for (int y = 0; y < VGA_BUFFER_HEIGHT; y++) {
            if (y < num) {
                int vx = (num > 0)
                    ? (int)(((float)VGA_BUFFER_HEIGHT / num) * y)
                    : 0;
                int x0 = SCREEN_WIDTH/2 + (int)(cosf(theta * (float)M_PI / 180.0f) * vx) - 5;
                int x1 = SCREEN_WIDTH/2 + (int)(cosf(theta * (float)M_PI / 180.0f) * vx) + 5;
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

    // Cleanup
    close(us_fd);
    close(vga_ball_fd);
    return 0;
}