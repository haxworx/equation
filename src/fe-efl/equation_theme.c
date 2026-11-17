/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) Enlightenment Development Team
 */

#include "equation_theme.h"
#include "equation_intl.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const Equation_Theme_Info equation_theme_catalog[3]
  = { { "observatory", N_("Observatory"), N_("Midnight instruments / cyan starlight") },
       { "atelier", N_("Atelier"), N_("Warm paper / a study in numbers") },
       { "crazy_lab", N_("Crazy Lab"), N_("Glass cathodes / high voltage arithmetic") } };

static bool
equation_theme_valid(const char *path)
{
   return path && edje_file_group_exists(path, "equation/frame")
          && edje_file_group_exists(path, "equation/display")
          && edje_file_group_exists(path, "equation/result-row")
          && edje_file_group_exists(path, "equation/icon/copy")
          && edje_file_group_exists(path, "elm/button/base/default");
}

bool
equation_theme_resolve(const char *name, char *path, size_t capacity)
{
   if (!name) return false;
   bool known = false;
   for (unsigned i = 0; i < EINA_C_ARRAY_LENGTH(equation_theme_catalog); ++i)
     if (!strcmp(name, equation_theme_catalog[i].name)) known = true;
   if (!known) return false;
   char executable[PATH_MAX];
   ssize_t length = readlink("/proc/self/exe", executable, sizeof(executable) - 1);
   bool development = false;
   if (length > 0)
     {
        executable[length] = 0;
        size_t root_length = strlen(EQUATION_BUILD_ROOT);
        development
          = !strncmp(executable, EQUATION_BUILD_ROOT, root_length) && executable[root_length] == '/';
     }
   int count;
   if (development)
     count = snprintf(path, capacity, "%s/%s/equation_%s.edj", EQUATION_THEME_BUILD_DIR, name, name);
   else
     count = snprintf(path, capacity, "%s/equation_%s.edj", EQUATION_THEME_INSTALL_DIR, name);
   return count >= 0 && (size_t)count < capacity && equation_theme_valid(path);
}

void
equation_theme_display_set(Evas_Object *display, const char *value)
{
   size_t length = strlen(value);
   bool digits = length > 0 && length <= 14;
   for (size_t i = 0; i < length; ++i)
     if (!strchr("0123456789.-+eE", value[i])) digits = false;
   elm_object_part_text_set(display, "equation.value", value);
   for (size_t i = 0; i < 14; ++i)
     {
        char part[40], digit[2] = { 0 };
        snprintf(part, sizeof(part), "equation.digit.%zu", i);
        if (digits && i + length >= 14) digit[0] = value[i + length - 14];
        if (edje_object_part_exists(elm_layout_edje_get(display), part))
          elm_object_part_text_set(display, part, digit);
     }
   elm_layout_signal_emit(display, digits ? "equation,digits" : "equation,compact", "equation");
}
