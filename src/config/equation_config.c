/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) Enlightenment Development Team
 */

#include "equation_config.h"
#include <Eet.h>
#include <Ecore_File.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
equation_config_read(Eet_File *file, const char *key, char *out, size_t capacity)
{
   int length = 0;
   char *value = eet_read(file, key, &length);
   if (value && length > 0 && (size_t)length <= capacity && value[length - 1] == 0
       && strlen(value) == (size_t)length - 1)
     memcpy(out, value, (size_t)length);
   free(value);
}

void
equation_config_load(Equation_Config *config)
{
   memset(config, 0, sizeof(*config));
   config->animations = true;
   snprintf(config->theme, sizeof(config->theme), "observatory");
   const char *base = getenv("XDG_CONFIG_HOME"), *home = getenv("HOME");
   int length;
   if (base && base[0] == '/')
     length = snprintf(config->path, sizeof(config->path), "%s/equation/equation.eet", base);
   else if (home && home[0] == '/')
     length = snprintf(config->path, sizeof(config->path), "%s/.config/equation/equation.eet", home);
   else
     return;
   if (length < 0 || (size_t)length >= sizeof(config->path))
     {
        config->path[0] = 0;
        return;
     }
   Eet_File *file = eet_open(config->path, EET_FILE_MODE_READ);
   if (!file) return;
   equation_config_read(file, "appearance/theme", config->theme, sizeof(config->theme));
   if (!config->theme[0]) snprintf(config->theme, sizeof(config->theme), "observatory");
   char setting[16] = "0";
   equation_config_read(file, "calculator/mode", setting, sizeof(setting));
   config->mode = !strcmp(setting, "1");
   snprintf(setting, sizeof(setting), "0");
   equation_config_read(file, "calculator/degrees", setting, sizeof(setting));
   config->degrees = !strcmp(setting, "1");
   snprintf(setting, sizeof(setting), "1");
   equation_config_read(file, "appearance/animations", setting, sizeof(setting));
   config->animations = strcmp(setting, "0") != 0;
   eet_close(file);
}

bool
equation_config_save(const Equation_Config *config)
{
   if (!config->path[0]) return false;
   char *directory = ecore_file_dir_get(config->path);
   bool ready = directory && ecore_file_mkpath(directory);
   free(directory);
   if (!ready) return false;
   char temporary[PATH_MAX];
   int length = snprintf(temporary, sizeof(temporary), "%s.XXXXXX", config->path);
   if (length < 0 || (size_t)length >= sizeof(temporary)) return false;
   int descriptor = mkstemp(temporary);
   if (descriptor < 0) return false;
   close(descriptor);
   Eet_File *file = eet_open(temporary, EET_FILE_MODE_WRITE);
   bool ok = false;
   if (file)
     {
        ok = eet_write(file, "appearance/theme", config->theme, strlen(config->theme) + 1, 1) > 0
             && eet_write(file, "calculator/mode", config->mode ? "1" : "0", 2, 1) > 0
             && eet_write(file, "calculator/degrees", config->degrees ? "1" : "0", 2, 1) > 0
             && eet_write(file, "appearance/animations", config->animations ? "1" : "0", 2, 1) > 0;
        if (eet_close(file) != EET_ERROR_NONE) ok = false;
     }
   if (ok && rename(temporary, config->path) == 0) return true;
   unlink(temporary);
   return false;
}
