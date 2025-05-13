#ifndef _ULTRASONIC_SENSOR_H
#define _ULTRASONIC_SENSOR_H

#include <linux/ioctl.h>

/**
 * Structure for writing a 16-bit chirp value.
 */
typedef struct {
    int chirp;
} ultrasonic_chirp_t;

/**
 * Structure for writing a 16-bit time signal value.
 */
typedef struct {
    int time_signal;
} ultrasonic_time_t;

/**
 * Structure for reading a 32-bit status value.
 */
typedef struct {
    int status;
} ultrasonic_status_t;

/**
 * Magic number for ultrasonic sensor ioctls.
 */
#define ULTRASONIC_SENSOR_MAGIC 'u'

/**
 * Write the chirp signal (16-bit). Argument: ultrasonic_chirp_t
 */
#define US_WRITE_CHIRP _IOW(ULTRASONIC_SENSOR_MAGIC, 1, ultrasonic_chirp_t)

/**
 * Write the time signal (16-bit). Argument: ultrasonic_time_t
 */
#define US_WRITE_TIME  _IOW(ULTRASONIC_SENSOR_MAGIC, 2, ultrasonic_time_t)

/**
 * Read the status (32-bit). Argument: ultrasonic_status_t
 */
#define US_READ_STATUS _IOR(ULTRASONIC_SENSOR_MAGIC, 3, ultrasonic_status_t)

#endif /* _ULTRASONIC_SENSOR_H */