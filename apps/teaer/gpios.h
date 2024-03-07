#ifndef TEAER_GPIOS_H
#define TEAER_GPIOS_H

int gpios_init(void);
void pump_on(void);
void pump_off(void);
void teapot_on(void);
void teapot_off(void);
void basket_elevate(void);
void basket_lower(void);
int start_get(void);

#endif
