/* ==========================================================================
    Licensed under BSD 2clause license See LICENSE file for more information
    Author: Michał Łyszczek <michal.lyszczek@bofc.pl>
   ==========================================================================
              ░▀█▀░█▀█░█▀▀░█░░░█░█░█▀▄░█▀▀░░░█▀▀░▀█▀░█░░░█▀▀░█▀▀
              ░░█░░█░█░█░░░█░░░█░█░█░█░█▀▀░░░█▀▀░░█░░█░░░█▀▀░▀▀█
              ░▀▀▀░▀░▀░▀▀▀░▀▀▀░▀▀▀░▀▀░░▀▀▀░░░▀░░░▀▀▀░▀▀▀░▀▀▀░▀▀▀
   ========================================================================== */
#include <nuttx/config.h>
#include <embedlog.h>
#include <fcntl.h>
#include <nuttx/analog/hx711.h>
#include <nuttx/sensors/sensor.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>


#include "utils.h"


/* ==========================================================================
               ░█▀▄░█▀▀░█▀▀░█░░░█▀█░█▀▄░█▀█░▀█▀░▀█▀░█▀█░█▀█░█▀▀
               ░█░█░█▀▀░█░░░█░░░█▀█░█▀▄░█▀█░░█░░░█░░█░█░█░█░▀▀█
               ░▀▀░░▀▀▀░▀▀▀░▀▀▀░▀░▀░▀░▀░▀░▀░░▀░░▀▀▀░▀▀▀░▀░▀░▀▀▀
   ========================================================================== */
static int g_hx711_fd;
static int g_temp_fd;


/* ==========================================================================
        ░█▀█░█░█░█▀▄░█░░░▀█▀░█▀▀░░░█▀▀░█░█░█▀█░█▀▀░▀█▀░▀█▀░█▀█░█▀█░█▀▀
        ░█▀▀░█░█░█▀▄░█░░░░█░░█░░░░░█▀▀░█░█░█░█░█░░░░█░░░█░░█░█░█░█░▀▀█
        ░▀░░░▀▀▀░▀▀░░▀▀▀░▀▀▀░▀▀▀░░░▀░░░▀▀▀░▀░▀░▀▀▀░░▀░░▀▀▀░▀▀▀░▀░▀░▀▀▀
   ==========================================================================
                       ╻ ╻┏━╸╻┏━╸╻ ╻╺┳╸   ╺┳╸┏━┓┏━┓┏━╸
                       ┃╻┃┣╸ ┃┃╺┓┣━┫ ┃     ┃ ┣━┫┣┳┛┣╸
                       ┗┻┛┗━╸╹┗━┛╹ ╹ ╹     ╹ ╹ ╹╹┗╸┗━╸
   ==========================================================================
    Tares the scale with specific precision. Precision is in grams.
   ========================================================================== */
int weight_tare
(
	float  precision   /* Precision with which tare the scale. In grams. */
)
{
	el_oprint(OELD, "");
	if (ioctl(g_hx711_fd, HX711_TARE, &precision))
		el_operror_return(OELF, -1, "Failed to tare the scale");

	return 0;
}

/* ==========================================================================
                       ┏━┓┏━╸┏┓╻┏━┓┏━┓┏━┓┏━┓   ╻┏┓╻╻╺┳╸
                       ┗━┓┣╸ ┃┗┫┗━┓┃ ┃┣┳┛┗━┓   ┃┃┗┫┃ ┃
                       ┗━┛┗━╸╹ ╹┗━┛┗━┛╹┗╸┗━┛╺━╸╹╹ ╹╹ ╹
   ==========================================================================
    Initializes and configures all sensors.
   ========================================================================== */
int sensors_init
(
	void
)
{
	int flags;  /* fcntl flags */
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


	/* Open devices */

	if ((g_hx711_fd = open("/dev/hx711_0", O_RDWR)) < 0)
		el_operror_return(OELF, -1, "can't open /dev/hx711_0");

	if ((g_temp_fd = open("/dev/uorb/sensor_temp0", O_RDONLY)) < 0)
		el_operror_return(OELF, -1, "can't open /dev/uorb/sensor_temp0");

	/* Configure hx711 driver, so that it returns us values in grams */

	if (ioctl(g_hx711_fd, HX711_SET_VAL_PER_UNIT, 703))
		el_operror_return(OELF, -1, "Can't set hx711 value per unit");

	/* Perform initial tare, we do it every time, but if something is
	 * wrong, we will know at boot */
	if (weight_tare(5))
		return -1;

	/* We us ds18d20 sensor without in kernel polling. If polling is
	 * disabled the only way to make this work is to make socket
	 * non blocking. Otherwise kernel will block us, and we will
	 * never unblock. Probably because there is no poll thread to
	 * post semaphore. */
	flags = fcntl(g_temp_fd, F_GETFL, 0);
	fcntl(g_temp_fd, F_SETFL, flags | O_NONBLOCK);

	return 0;
}

/* ==========================================================================
                         ╻ ╻┏━╸╻┏━╸╻ ╻╺┳╸   ┏━╸┏━╸╺┳╸
                         ┃╻┃┣╸ ┃┃╺┓┣━┫ ┃    ┃╺┓┣╸  ┃
                         ┗┻┛┗━╸╹┗━┛╹ ╹ ╹ ╺━╸┗━┛┗━╸ ╹
   ==========================================================================
    Returns current weight readings in grams. If there is an error, INT_MIN
    is returned.
   ========================================================================== */
int weight_get
(
	void
)
{
	int32_t weight;  /* weight, in grams, read from the driver */
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


	if (read(g_hx711_fd, &weight, sizeof(weight)) != sizeof(weight))
		return INT_MIN;

	el_oprint(OELD, "%d", (int)weight);
	return weight;
}



/* ==========================================================================
                           ╺┳╸┏━╸┏┳┓┏━┓   ┏━╸┏━╸╺┳╸
                            ┃ ┣╸ ┃┃┃┣━┛   ┃╺┓┣╸  ┃
                            ╹ ┗━╸╹ ╹╹     ┗━┛┗━╸ ╹
   ==========================================================================
    Gets current temperature readings in °C. If there is an error, INT_MIN
    is returned instead.
   ========================================================================== */
int temp_get
(
	void
)
{
	struct sensor_temp temp;
	if (read(g_temp_fd, &temp, sizeof(temp)) < 1)
		return INT_MIN;

	el_oprint(OELD, "%d", (int)temp.temperature);
	/* temperature is float, but we only really care for 1°C of precision */
	return temp.temperature;
}
