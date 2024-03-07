/* ==========================================================================
    Licensed under BSD 2clause license See LICENSE file for more information
    Author: Michał Łyszczek <michal.lyszczek@bofc.pl>
   ========================================================================== */

#ifndef TEAER_SENSORS_H
#define TEAER_SENSORS_H

int sensors_init(void);
int weight_tare(float precision);
int weight_get(void);
int temp_get(void);

#endif /* TEAER_SENSORS_H */
