#ifndef ULTRASONIC_SENSOR_H
#define ULTRASONIC_SENSOR_H

#include <stdint.h>
#include <linux/ioctl.h>
#include <linux/types.h>

#define ULTRASONIC_MAGIC  'u'

/**
 * US_WRITE_CONFIG
 *   Write a 32-bit configuration word:
 *     [31:16] = timeout value (16-bit)
 *     [15: 0] = chirp bit (0 or 1)
 */
#define US_WRITE_CONFIG  _IOW(ULTRASONIC_MAGIC, 1, __u32)

/**
 * US_READ_STATUS
 *   Read a 32-bit status word:
 *     all-1s = timeout
 *     0       = still waiting
 *     else    = echo length
 */
#define US_READ_STATUS   _IOR(ULTRASONIC_MAGIC, 2, uint32_t)

/* Device name */
#define ULTRASONIC_DEV_NAME  "ultrasonic_sensor"

#endif
