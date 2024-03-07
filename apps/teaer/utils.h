/* ==========================================================================
    Licensed under BSD 2clause license See LICENSE file for more information
    Author: Michał Łyszczek <michal.lyszczek@bofc.pl>
   ========================================================================== */

#ifndef TEAER_UTILS_H
#define TEAER_UTILS_H

#define el_operror_return(LEVEL, r, ...) do { \
		el_operror(LEVEL, __VA_ARGS__); \
		return r; \
	} while (0)

#define EL_OPTIONS_OBJECT &g_teaer_el
extern struct el g_teaer_el;

#endif /* TEAER_UTILS_H */
