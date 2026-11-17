/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) Enlightenment Development Team
 */

#ifndef EQUATION_WINDOW_H
#define EQUATION_WINDOW_H
#include <Elementary.h>
Evas_Object *equation_window_add(Evas_Object *parent, const char *title, const char *path,
                                 Elm_Theme *theme);
void equation_window_theme_set(Evas_Object *window, const char *path);
void equation_window_content_set(Evas_Object *window, Evas_Object *content);
void equation_window_menu_set(Evas_Object *window, Evas_Smart_Cb callback, void *data);
void equation_window_size_minimum(Evas_Object *window);
#endif
