/* ==========================================================================
    Licensed under BSD 2clause license See LICENSE file for more information
    Author: Michał Łyszczek <michal.lyszczek@bofc.pl>
   ==========================================================================
              ░▀█▀░█▀█░█▀▀░█░░░█░█░█▀▄░█▀▀░░░█▀▀░▀█▀░█░░░█▀▀░█▀▀
              ░░█░░█░█░█░░░█░░░█░█░█░█░█▀▀░░░█▀▀░░█░░█░░░█▀▀░▀▀█
              ░▀▀▀░▀░▀░▀▀▀░▀▀▀░▀▀▀░▀▀░░▀▀▀░░░▀░░░▀▀▀░▀▀▀░▀▀▀░▀▀▀
   ========================================================================== */
#include <nuttx/config.h>
#include <stdio.h>
#include <embedlog.h>
#include <nuttx/analog/hx711.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <nuttx/sensors/sensor.h>
#include <ctype.h>

#include "sensors.h"
#include "utils.h"
#include "gpios.h"


/* ==========================================================================
               ░█▀▄░█▀▀░█▀▀░█░░░█▀█░█▀▄░█▀█░▀█▀░▀█▀░█▀█░█▀█░█▀▀
               ░█░█░█▀▀░█░░░█░░░█▀█░█▀▄░█▀█░░█░░░█░░█░█░█░█░▀▀█
               ░▀▀░░▀▀▀░▀▀▀░▀▀▀░▀░▀░▀░▀░▀░▀░░▀░░▀▀▀░▀▀▀░▀░▀░▀▀▀
   ========================================================================== */

struct el g_teaer_el;
static FILE *g_ctl_uart_f;

struct tea_params
{
	int water_amount;
	int brew_time;
	int temp;
};

/* ==========================================================================
      ░█▀█░█▀▄░▀█▀░█░█░█▀█░▀█▀░█▀▀░░░█▀▀░█░█░█▀█░█▀▀░▀█▀░▀█▀░█▀█░█▀█░█▀▀
      ░█▀▀░█▀▄░░█░░▀▄▀░█▀█░░█░░█▀▀░░░█▀▀░█░█░█░█░█░░░░█░░░█░░█░█░█░█░▀▀█
      ░▀░░░▀░▀░▀▀▀░░▀░░▀░▀░░▀░░▀▀▀░░░▀░░░▀▀▀░▀░▀░▀▀▀░░▀░░▀▀▀░▀▀▀░▀░▀░▀▀▀
   ==========================================================================
                            ┏━╸┏━╸╺┳╸   ╻  ╻┏┓╻┏━╸
                            ┃╺┓┣╸  ┃    ┃  ┃┃┗┫┣╸
                            ┗━┛┗━╸ ╹    ┗━╸╹╹ ╹┗━╸
   ==========================================================================
    Reads single line from the serial device. If $line buffer is not big
    enough to hold a line, function will return error, and $line will not
    hold whole line.
   ========================================================================== */
int get_line
(
	char    *line,    /* */
	size_t   linelen  /* */
)
{
	line[linelen - 1] = 0x7f;

	/* serial device cannot return end of file, so we don't even check it */
	if (fgets(line, linelen, g_ctl_uart_f) == NULL)
		el_operror_return(OELW, -1, "Failed to read from serial device");

	if (line[linelen - 1] == '\0' && line[linelen - 2] != '\n')
	{
		/* discard whole line not much we can do with it anyway */
		while ((getc(g_ctl_uart_f)) != '\n');
		return -1;
	}

	return 0;
}

/* ==========================================================================
                   ╻ ╻┏━┓╻╺┳╸   ┏━╸┏━┓┏━┓   ┏━┓╺┳╸┏━┓┏━┓╺┳╸
                   ┃╻┃┣━┫┃ ┃    ┣╸ ┃ ┃┣┳┛   ┗━┓ ┃ ┣━┫┣┳┛ ┃
                   ┗┻┛╹ ╹╹ ╹    ╹  ┗━┛╹┗╸   ┗━┛ ╹ ╹ ╹╹┗╸ ╹
   ==========================================================================
   ========================================================================== */
static void wait_for_start
(
	struct tea_params *params
)
{
	char buf[128];
	char *b;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


	params->water_amount = 400; /* [ml] */
	params->temp = 80; /* [°C] */
	params->brew_time = 60; /* [s] */
	b = buf;

	for (;;)
	{
		if (get_line(buf, sizeof(buf)))
			continue;

		if (strncmp(b, "start", strlen("start")) != 0)
			continue;

		b += strlen("start ");
		params->brew_time = atoi(b);
		return;
	}
}


/* ==========================================================================
                         ┏━╸╺┳╸╻     ┏━┓┏━╸┏━┓╻  ╻ ╻
                         ┃   ┃ ┃     ┣┳┛┣╸ ┣━┛┃  ┗┳┛
                         ┗━╸ ╹ ┗━╸   ╹┗╸┗━╸╹  ┗━╸ ╹
   ==========================================================================
    Called by embedlog as custom put function. We send out logs over serial
    line.
   ========================================================================== */
int ctl_reply
(
	const char  *s,     /* */
	size_t       slen,  /* */
	void        *user   /* */
)
{
	FILE *f =    user;
	const char  *log_level;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


	fwrite(s, slen, 1, f);
	return 0;

	log_level = s;

	/* Find first space, after first space there is log level info */
	while (!isspace(*log_level++));
	/* log_level now points to log level */
	if (*log_level == 'd' || *log_level == 'i')
		return 0;

	/* We are only sending notice or higher */
	fwrite(s, slen, 1, f);
	return 0;
}


/* ==========================================================================
                               ░█▄█░█▀█░▀█▀░█▀█
                               ░█░█░█▀█░░█░░█░█
                               ░▀░▀░▀░▀░▀▀▀░▀░▀
   ========================================================================== */
int main(int argc, FAR char *argv[])
{
	struct tea_params params;
	time_t start;
	(void)argc;
	(void)argv;

	/* Initialize logger */
	el_oinit(&g_teaer_el);
	/* Show seconds from boot in logs, no factions, we are too slow
	 * for that */
	el_oset_timestamp(&g_teaer_el, EL_TS_SHORT, EL_TS_TM_TIME, 0);
	el_oprint_extra_info(&g_teaer_el, 1);
	el_oenable_colors(&g_teaer_el, 1);

	el_oset_log_level(&g_teaer_el, EL_DBG);

	/* Initialize control uart */

	if ((g_ctl_uart_f = fopen("/dev/ttyS1", "r+")) == NULL)
		el_operror_return(OELF, 1, "Can't open control uart");

	/* Set custom print for embedlog - we will send errors and
	 * notifications back to control socket */
	el_oset_custom_put(&g_teaer_el, ctl_reply, g_ctl_uart_f);

	/* Initialize sensors and gpios */

	if (sensors_init())
		return 1;
	if (gpios_init())
		return 1;

	for (;;)
	{
		#define alarm(...) do { el_oprint(__VA_ARGS__); goto error; } while (0)

		/* Wait for start signal, be that button, or network signal */
		wait_for_start(&params);

		el_oprint(OELI, "Start");
		/* Check for current weight, if we detect something like 50g
		 * or more, we assume there might be some water leftover, we
		 * cannot work in those conditions as we risk overfilling pot */
		if (weight_get() > 50)
			alarm(OELW, "Weight is >50g, is there water left in teapot?");

		/* Tare the scale, weight may change a little bit, depending
		 * on teapot position and other factorio... this shit is not
		 * super stable :P */
		if (weight_tare(5))
			alarm(OELW, "We couldn't tare the scale");

		el_oprint(OELI, "Fill teapot with water");
		/* Enable water pump, to pour water into the teapot */
		pump_on();

		/* wait for water to fill the teapot by checking the weight */
		for (start = time(NULL);;)
		{
			if (time(NULL) - start > 7)
			/* failsafe, if scale dies on us while pouring water,
			 * we may flood the room, abort when we cannot get proper
			 * reading from scale */
				alarm(OELW, "Error while filling teapot");

			if (weight_get() > params.water_amount)
				break;
		}

		el_oprint(OELI, "Start heating");
		/* water is in the teapot, turn of pump */
		pump_off();
		/* and start heating */
		teapot_on();

		/* If we are making consecutive tea in quick succession, it
		 * is possible that thermometer will be hot from previous
		 * brewing and we will immediately jump off heating step.
		 * Wait until sensor cools down below threshold to be sure
		 * we don't jump over */
		for (start = time(NULL);;)
		{
			/* 10 seconds is way more then enough for sensor
			 * to cool down, if for some reason someone but if
			 * for some bizzare reason someone pours hot water
			 * right from the start, we continue after these
			 * 10 seconds */
			if (time(NULL) - start > 10)
				break;

			/* exit loop of temperature is below threshold */
			if (temp_get() < params.temp - 5)
				break;
		}

		for (start = time(NULL);;)
		{
			//if (time(NULL) - start > 5) break; /* DEBUG */
			/* failsafe, if thermometr dies on us while heating, we
			 * may cause fire by heating forever. Teapot won't turn
			 * itself off, if bimetal is removed. */
			if (time(NULL) - start > 300)
				alarm(OELW, "Error while heating water");

			/* There is a lot o inertia during boiling, so we
			 * wait for T - 5°C, and then we turn off the kettle.
			 * Due to inertia temp will go higher anyway */
			if (temp_get() > params.temp - 5)
				break;
		}

		teapot_off();

		for (start = time(NULL);;)
		{
			/* In case we overdid it, and inertia won't boil water
			 * for us as expected, we will just continue */
			if (time(NULL) - start > 60)
				break;

			/* Wait for actual desired temp - with kettle off */
			if (temp_get() > params.temp)
				break;
		}

		el_oprint(OELI, "Wait for cool");

		/* now, due to inertia, it is likely that temperature will
		 * go above threshold, so let's wait for it to cool off.
		 * We also wait for a little lower temperature than when
		 * heating, to introduce some hysterysis to prevent jumping
		 * over check, when temperature oscilates. */

		for (start = time(NULL);;)
		{
			//if (time(NULL) - start > 5) break; /* DEBUG */
			/* Semi failsafe, nothing can happen, but we want alarm
			 * if temp sensor dies or anything happens */
			if (time(NULL) - start > 1800)
				alarm(OELW, "Error waiting for water to cool off");

			if (temp_get() < params.temp - 2)
				break;
		}

		el_oprint(OELI, "Lower payload");
		/* Temperature is ok, lower the payload! */
		basket_lower();

		/* wait for tea to brew */
		sleep(params.brew_time);

		el_oprint(OELI, "elevate paylaod");
		basket_elevate();

		/* Tea is done, but drinking tea when its temperature
		 * is ~80 or even 90°C is no fun, so wait for good
		 * tea temperature before sending notification */
		for (start = time(NULL);;)
		{
			if (time(NULL) - start > 1800)
				break;

			if (temp_get() < 60)
				break;
		}

		el_oprint(OELN, "sir, the tea is done!");

		/* done :] */
error:
		/* turn off all stuff in case of an error */
		pump_off();
		teapot_off();
	}

	return 1;
}
