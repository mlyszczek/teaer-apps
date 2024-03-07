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
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <nuttx/ioexpander/gpio.h>

#include "utils.h"
#include "gpios.h"

/* ==========================================================================
               ░█▀▄░█▀▀░█▀▀░█░░░█▀█░█▀▄░█▀█░▀█▀░▀█▀░█▀█░█▀█░█▀▀
               ░█░█░█▀▀░█░░░█░░░█▀█░█▀▄░█▀█░░█░░░█░░█░█░█░█░▀▀█
               ░▀▀░░▀▀▀░▀▀▀░▀▀▀░▀░▀░▀░▀░▀░▀░░▀░░▀▀▀░▀▀▀░▀░▀░▀▀▀
   ========================================================================== */
static int g_gpio_teapot_fd;
static int g_gpio_pump_fd;
static int g_gpio_motor_left_fd;
static int g_gpio_motor_right_fd;
static int g_gpio_start_fd;


/* ==========================================================================
        ░█▀█░█░█░█▀▄░█░░░▀█▀░█▀▀░░░█▀▀░█░█░█▀█░█▀▀░▀█▀░▀█▀░█▀█░█▀█░█▀▀
        ░█▀▀░█░█░█▀▄░█░░░░█░░█░░░░░█▀▀░█░█░█░█░█░░░░█░░░█░░█░█░█░█░▀▀█
        ░▀░░░▀▀▀░▀▀░░▀▀▀░▀▀▀░▀▀▀░░░▀░░░▀▀▀░▀░▀░▀▀▀░░▀░░▀▀▀░▀▀▀░▀░▀░▀▀▀
   ==========================================================================
                           ┏━╸┏━┓╻┏━┓┏━┓   ╻┏┓╻╻╺┳╸
                           ┃╺┓┣━┛┃┃ ┃┗━┓   ┃┃┗┫┃ ┃
                           ┗━┛╹  ╹┗━┛┗━┛   ╹╹ ╹╹ ╹
   ==========================================================================
    Initializes all GPIO (heater, pump, motor, buttons etc) to work.
   ========================================================================== */
int gpios_init
(
	void
)
{
	if ((g_gpio_teapot_fd = open("/dev/gpio_teapot", O_RDWR)) < 0)
		el_operror_return(OELF, -1, "can't open /dev/gpio_teapot");

	if ((g_gpio_pump_fd = open("/dev/gpio_pump", O_RDWR)) < 0)
		el_operror_return(OELF, -1, "can't open /dev/gpio_pump");

	if ((g_gpio_motor_left_fd = open("/dev/gpio_motor_l", O_RDWR)) < 0)
		el_operror_return(OELF, -1, "can't open /dev/gpio_motor_l");

	if ((g_gpio_motor_right_fd = open("/dev/gpio_motor_r", O_RDWR)) < 0)
		el_operror_return(OELF, -1, "can't open /dev/gpio_motor_r");

#if 0
	if ((g_gpio_start_fd = open("/dev/gpio_start", O_RDWR)) < 0)
		el_operror_return(OELF, -1, "can't open /dev/gpio_start");
#endif

	return 0;
}



/* ==========================================================================
                     ┏┓ ┏━┓┏━┓╻┏ ┏━╸╺┳╸   ╻  ┏━┓╻ ╻┏━╸┏━┓
                     ┣┻┓┣━┫┗━┓┣┻┓┣╸  ┃    ┃  ┃ ┃┃╻┃┣╸ ┣┳┛
                     ┗━┛╹ ╹┗━┛╹ ╹┗━╸ ╹    ┗━╸┗━┛┗┻┛┗━╸╹┗╸
   ==========================================================================
   ========================================================================== */
void basket_lower
(
	void
)
{
	el_oprint(OELD, "1");
	write(g_gpio_motor_right_fd, "1", 1);
	/* TODO, probably would be better to add some limiter button */
	usleep(3.5 * 1000 * 1000);
	el_oprint(OELD, "0");
	write(g_gpio_motor_right_fd, "0", 1);
}


/* ==========================================================================
                  ┏┓ ┏━┓┏━┓╻┏ ┏━╸╺┳╸   ┏━╸╻  ┏━╸╻ ╻┏━┓╺┳╸┏━╸
                  ┣┻┓┣━┫┗━┓┣┻┓┣╸  ┃    ┣╸ ┃  ┣╸ ┃┏┛┣━┫ ┃ ┣╸
                  ┗━┛╹ ╹┗━┛╹ ╹┗━╸ ╹    ┗━╸┗━╸┗━╸┗┛ ╹ ╹ ╹ ┗━╸
   ==========================================================================
   ========================================================================== */
void basket_elevate
(
	void
)
{
	el_oprint(OELD, "1");
	write(g_gpio_motor_left_fd, "1", 1);
	/* TODO, probably would be better to add some limiter button */
	usleep(3.5 * 1000 * 1000);
	el_oprint(OELD, "0");
	write(g_gpio_motor_left_fd, "0", 1);
}


#if 0
/* ==========================================================================
                         ┏━┓╺┳╸┏━┓┏━┓╺┳╸   ┏━╸┏━╸╺┳╸
                         ┗━┓ ┃ ┣━┫┣┳┛ ┃    ┃╺┓┣╸  ┃
                         ┗━┛ ╹ ╹ ╹╹┗╸ ╹    ┗━┛┗━╸ ╹
   ==========================================================================
   ========================================================================== */
int start_get
(
	void
)
{
	bool start_pin = 0;
	ioctl(g_gpio_start_fd, GPIOC_READ, (unsigned long)((uintptr_t)&start_pin));
	el_oprint(OELD, "start: %d", start_pin);
	return start_pin;
}
#endif

/* ==========================================================================
                ┏━┓╻┏┳┓┏━┓╻  ┏━╸   ┏━╸┏━┓╻┏━┓   ╺┳┓┏━┓╻╻ ╻┏━╸
                ┗━┓┃┃┃┃┣━┛┃  ┣╸    ┃╺┓┣━┛┃┃ ┃    ┃┃┣┳┛┃┃┏┛┣╸
                ┗━┛╹╹ ╹╹  ┗━╸┗━╸   ┗━┛╹  ╹┗━┛   ╺┻┛╹┗╸╹┗┛ ┗━╸
   ==========================================================================
    Trivial, onliner, functions to drive gpios. They don't deserve special
    place. Pump and teapot gpios are turned on with "low" gpio.
   ========================================================================== */
void pump_on(void)    { el_oprint(OELD, "1"); write(g_gpio_pump_fd,   "0", 1); }
void pump_off(void)   { el_oprint(OELD, "0"); write(g_gpio_pump_fd,   "1", 1); }
void teapot_on(void)  { el_oprint(OELD, "1"); write(g_gpio_teapot_fd, "0", 1); }
void teapot_off(void) { el_oprint(OELD, "0"); write(g_gpio_teapot_fd, "1", 1); }
