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
            chirp = 999;
        } else if (angle == 177) {
            chirp = 0;
        }

        // Pack 32-bit config: [31:16]=time, [15:0]=chirp
        // 3000000 nanoseconds = 60 milliseconds
        uint16_t timeout = 65535;
        uint32_t cfg = ((timeout & 0xFFFF) << 16) | (chirp & 0x1);
        printf("Writing config: timeout=0x%04x, chirp=%d, cfg=0x%08x\n", timeout, chirp, cfg);
        if (ioctl(us_fd, US_WRITE_CONFIG, &cfg) < 0) {
            perror("US_WRITE_CONFIG failed");
            break;
        }

        // Every 10°, read and print status
        if (angle % 10 == 0) {
            uint32_t status;
            if (ioctl(us_fd, US_READ_STATUS, &status) < 0) {
                perror("US_READ_STATUS failed");
                break;
            }
            printf("Echo status @ %3d° = 0x%08x, chirp = %d\n", angle, status, chirp);
        }


        int AngleDistanceFrom90 = fabs(90 - angle) / 30;

        // Define 3 simulated objects in the scene
        struct {
            int x;
            int y;
        } objects[] = {
            {SCREEN_WIDTH/4, VGA_BUFFER_HEIGHT/4},      // Left side, upper quarter
            {SCREEN_WIDTH/2, VGA_BUFFER_HEIGHT/2},      // Center, middle
            {3*SCREEN_WIDTH/4, 3*VGA_BUFFER_HEIGHT/4}   // Right side, lower quarter
        };

        // Compute line geometry
        int num = (int)(VGA_BUFFER_HEIGHT * sin(theta * (float)M_PI / 180.0f));
        if (num < 0) num = 0;
        if (num > SCREEN_HEIGHT) num = SCREEN_HEIGHT;

        // Check for intersections with simulated objects
        float angle_rad = theta * (float)M_PI / 180.0f;
        float cos_theta = cos(angle_rad);
        float sin_theta = sin(angle_rad);
        
        // Start with the full line length
        int original_num = num;
        
        // Check each object for intersection
        for (int i = 0; i < sizeof(objects)/sizeof(objects[0]); i++) {
            // Calculate object position relative to radar center
            int rel_x = objects[i].x - SCREEN_WIDTH/2;
            int rel_y = objects[i].y;
            
            // Calculate distance to object along the ray direction
            float dot_product = rel_x * cos_theta + rel_y * sin_theta;
            
            // Only consider objects in front of the radar
            if (dot_product > 0) {
                // Calculate perpendicular distance from ray to object (for collision detection)
                float perp_dist = fabs(rel_x * sin_theta - rel_y * cos_theta);
                
                // If object is close enough to the ray, consider it an intersection
                if (perp_dist < 15) {  // 15 pixels tolerance for "hitting" an object
                    // Convert dot_product to VGA_BUFFER_HEIGHT scale
                    int hit_distance = (int)(dot_product / VGA_BUFFER_HEIGHT * original_num);
                    
                    // If this hit is closer than current line length, shorten the line
                    if (hit_distance < num) {
                        num = hit_distance;
                        // Optional: Change line color or thickness at intersection point
                    }
                }
            }
        }

        for (int y = 0; y < VGA_BUFFER_HEIGHT; y++) {
            if (y < num) {
                int vx = (num > 0)
                    ? (int)(((float)VGA_BUFFER_HEIGHT / num) * y)
                    : 0;
                int x0 = SCREEN_WIDTH/2 + (int)(cos(theta * (float)M_PI / 180.0f) * vx) - (2 + AngleDistanceFrom90);
                int x1 = SCREEN_WIDTH/2 + (int)(cos(theta * (float)M_PI / 180.0f) * vx) + (2 + AngleDistanceFrom90);
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