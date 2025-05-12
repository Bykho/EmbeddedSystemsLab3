/*
 * Stephen A. Edwards
 * Columbia University
 */

//testing push
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <math.h>
#include "vga_ball.h"

/*
// Read and print the background color 
void print_background_color() {
  vga_ball_arg_t vla;
  
  if (ioctl(vga_ball_fd, VGA_BALL_READ_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_READ_BACKGROUND) failed");
      return;
  }
  printf("%02x %02x %02x\n",
	 vla.background.red, vla.background.green, vla.background.blue);
}

// Set the background color 
void set_background_color(const vga_ball_color_t *c)
{
  vga_ball_arg_t vla;
  vla.background = *c;
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_BACKGROUND, &vla)) {
      perror("ioctl(VGA_BALL_SET_BACKGROUND) failed");
      return;
  }
}
*/

//#define SCREEN_WIDTH 640
//#define SCREEN_HEIGHT 480
#define SCREEN_WIDTH 578
#define SCREEN_HEIGHT 449
#define BALL_SPEED 3
#define SLEEP_TIME 50000 //50ms delay between updates

//RADIUS IS 31

int main()
{
    printf("DEBUG: Starting main()\n");
    
    int vga_ball_fd;
    vga_ball_line_t vla_line;
    static const char filename[] = "/dev/vga_ball";

    // Ball position and velocity
    float theta = 1;
    int delta = 5;

    printf("DEBUG: Initializing LineMatrix\n");
    int LineMatrix[256][2];
    int i;
    for (i = 0; i < 256; i++) {
        LineMatrix[i][0] = 240;
        LineMatrix[i][1] = 240;
    }
    printf("DEBUG: LineMatrix initialized\n");

    printf("VGA ball Userspace program started\n");

    printf("DEBUG: Attempting to open device file: %s\n", filename);
    if ((vga_ball_fd = open(filename, O_RDWR)) == -1) {
        fprintf(stderr, "could not open %s\n", filename);
        return -1;
    }
    printf("DEBUG: Device file opened successfully\n");

    // Main animation loop
    printf("DEBUG: Entering main loop\n");
    while (1) {
        printf("DEBUG: Loop iteration start, theta = %f\n", theta);
        // Update position
        if (theta >= 178) {
            theta = 1;
        } else {
            theta += 1;
        }
        
        int max_y = (int) (256 * sin(theta * 3.14159265 / 180.0));
        printf("DEBUG: max_y calculated: %d\n", max_y);

        int y;
        for (y = 0; y < 480; y++) {
            if (y < max_y) {
                // cosine of theta times the number of x values we have, times the x value we are on.
                int virtual_x = ((float) (float)256/(float)(max_y)) * y;
                printf("DEBUG: Processing y=%d, virtual_x=%d\n", y, virtual_x);

                LineMatrix[y][0] = 320 + (int) (((float) (256 * cos(theta * 3.14159265 / 180.0))) * virtual_x) - delta;
                LineMatrix[y][1] = 320 + (int) (((float) (256 * cos(theta * 3.14159265 / 180.0))) * virtual_x) + delta;
                
                printf("DEBUG: LineMatrix[%d] values: [%d, %d]\n", y, LineMatrix[y][0], LineMatrix[y][1]);
            } else {
                LineMatrix[y][0] = -1;
                LineMatrix[y][1] = -1;
            }
        }

        printf("DEBUG: About to copy LineMatrix to vla_line\n");
        memcpy(vla_line.LineMatrix, LineMatrix, sizeof(LineMatrix));
        printf("DEBUG: Matrix copied successfully\n");

        printf("DEBUG: About to perform ioctl call\n");
        if (ioctl(vga_ball_fd, VGA_BALL_WRITE_LINE, &vla_line)) {
            perror("ioctl(VGA_BALL_WRITE_LINE) failed");
            printf("DEBUG: ioctl failed\n");
            break;
        }
        printf("DEBUG: ioctl successful\n");

        // Small delay to control animation speed
        usleep(SLEEP_TIME);
    }

    printf("VGA ball userspace program terminating\n");
    printf("DEBUG: Closing file descriptor\n");
    close(vga_ball_fd);
    return 0;
}
