/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) Enlightenment Development Team
 */

#ifndef EQUATION_THEME_H
#define EQUATION_THEME_H
#include <Elementary.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct
{
   const char *name, *label, *description;
} Equation_Theme_Info;
extern const Equation_Theme_Info equation_theme_catalog[3];
bool equation_theme_resolve(const char *name, char *path, size_t capacity);
void equation_theme_display_set(Evas_Object *display, const char *value);
#endif
