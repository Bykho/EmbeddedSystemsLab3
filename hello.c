/*
 * through ioctls
 *
 * Stephen A. Edwards
 * Columbia University
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
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

#define SCREEN_WIDTH 640 //578
#define SCREEN_HEIGHT 480
#define BALL_RADIUS 15
#define BALL_SPEED 3
#define SLEEP_TIME 25000  // 50ms delay between updates

//RADIUS IS 31

int main()
{
    int vga_ball_fd;
    vga_ball_arg_t vla;
    static const char filename[] = "/dev/vga_ball";

    // Ball position and velocity
    int x = 100;
    int y = 100;
    int dx = BALL_SPEED;  // Start moving right
    int dy = BALL_SPEED;  // Start moving down

    printf("VGA ball Userspace program started\n");

    if ((vga_ball_fd = open(filename, O_RDWR)) == -1) {
        fprintf(stderr, "could not open %s\n", filename);
        return -1;
    }

    // Main animation loop
    while (1) {
        // Update position
        x += dx;
        y += dy;

        // Bounce off walls
        if (x >= (SCREEN_WIDTH - BALL_RADIUS) + 1 || x <= BALL_RADIUS - 2) { 
            dx = -dx;  // Reverse X direction
            x += dx;   // Prevent sticking to wall
        }
        if (y >= (SCREEN_HEIGHT - BALL_RADIUS) + 1 || y <= BALL_RADIUS) {
            dy = -dy;  // Reverse Y direction
            y += dy;   // Prevent sticking to wall
        }

        // Update ball position in hardware
        vla.position.x = x;
        vla.position.y = y;

        if (ioctl(vga_ball_fd, VGA_BALL_WRITE_POSITION, &vla)) {
            perror("ioctl(VGA_BALL_WRITE_POSITION) failed");
            break;
        }

        // Small delay to control animation speed
        usleep(SLEEP_TIME);
    }

    printf("VGA ball userspace program terminating\n");
    close(vga_ball_fd);
    return 0;
}