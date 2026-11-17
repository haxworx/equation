/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) Enlightenment Development Team
 */

#ifndef EQUATION_UI_PRIVATE_H
#define EQUATION_UI_PRIVATE_H
#include <Elementary.h>
#include "equation_config.h"
typedef struct
{
   Evas_Object *window, *entry, *mode, *degrees, *scientific, *programmer, *status;
   Evas_Object *box, *common, *function_box, *base_box, *input_base;
   Evas_Object *results[2], *result_rows[2];
   Evas_Object *copy, *copy_icon;
   Ecore_Timer *copy_timer;
   bool result_valid, copy_success, motion_running;
   Evas_Object *menu, *about, *theme_status;
   Elm_Theme *theme;
   Evas_Object *display;
   Equation_Config config;
   char theme_path[PATH_MAX];
   char display_value[128];
   bool ready, config_failed;
} Equation_Ui;
#endif
