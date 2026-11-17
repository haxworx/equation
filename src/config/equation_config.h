/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) Enlightenment Development Team
 */

#ifndef EQUATION_CONFIG_H
#define EQUATION_CONFIG_H
#include <stdbool.h>
#include <limits.h>

typedef struct
{
   char path[PATH_MAX];
   char theme[PATH_MAX];
   int mode;
   bool degrees;
   bool animations;
} Equation_Config;

void equation_config_load(Equation_Config *config);
bool equation_config_save(const Equation_Config *config);
#endif
