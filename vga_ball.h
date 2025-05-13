#ifndef _VGA_BALL_H
#define _VGA_BALL_H

#include <linux/ioctl.h>

typedef struct {
  int x, y;
} vga_ball_position_t;
  

typedef struct {
  int LineMatrix[128][2];
} vga_ball_line_t;

typedef struct {
  vga_ball_position_t position;
  vga_ball_line_t line;
} vga_ball_arg_t;

#define VGA_BALL_MAGIC 'q'

/* ioctls and their arguments */
#define VGA_BALL_WRITE_POSITION _IOW(VGA_BALL_MAGIC, 1, vga_ball_arg_t)
#define VGA_BALL_READ_POSITION  _IOR(VGA_BALL_MAGIC, 2, vga_ball_arg_t)
#define VGA_BALL_WRITE_LINE _IOW(VGA_BALL_MAGIC, 3, vga_ball_line_t)
#define VGA_BALL_READ_LINE  _IOR(VGA_BALL_MAGIC, 4, vga_ball_line_t)

#endif
