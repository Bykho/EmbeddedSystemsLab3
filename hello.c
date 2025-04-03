
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

int main()
{
  int vga_ball_fd;
  vga_ball_arg_t vla;
  static const char filename[] = "/dev/vga_ball";

  printf("VGA ball Userspace program started\n");

  /*
  static const vga_ball_color_t colors[] = {
    { 0xff, 0x00, 0x00 }, //Red 
    { 0x00, 0xff, 0x00 }, // Green
    { 0x00, 0x00, 0xff }, // Blue
    { 0xff, 0xff, 0x00 }, // Yellow
    { 0x00, 0xff, 0xff }, // Cyan
    { 0xff, 0x00, 0xff }, // Magenta
    { 0x80, 0x80, 0x80 }, // Gray
    { 0x00, 0x00, 0x00 }, // Black
    { 0xff, 0xff, 0xff }  // White
  };
  */


  if ((vga_ball_fd = open(filename, O_RDWR)) == -1) {
      fprintf(stderr, "could not open %s\n", filename);
      return -1;
  }

  // Set ball position to center of screen
  vla.position.x = 30;  // Assuming 640x480 display
  vla.position.y = 30;

  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_POSITION, &vla)) {
      perror("ioctl(VGA_BALL_WRITE_POSITION) failed");
      return -1;
  }

  printf("Ball position set to (%d,%d)\n", vla.position.x, vla.position.y);
    
  // Keep the program running to maintain the display
  printf("Press enter to exit...\n");
  getchar();

  printf("VGA ball userspace program terminating\n");
  close(vga_ball_fd);
  return 0;
}