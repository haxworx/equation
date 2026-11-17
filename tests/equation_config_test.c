/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) Enlightenment Development Team
 */

#include "equation_config.h"
#include <Eet.h>
#include <Ecore_File.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EQUATION_CHECK(test)                                                                       \
  do                                                                                               \
    {                                                                                              \
      if (!(test))                                                                                 \
        {                                                                                          \
          fprintf(stderr, "Check failed at %d: %s\n", __LINE__, #test);                            \
          return 1;                                                                                \
        }                                                                                          \
    }                                                                                              \
  while (0)

int
main(void)
{
   eet_init();
   ecore_file_init();
   char directory[] = "/tmp/equation-config-XXXXXX";
   EQUATION_CHECK(mkdtemp(directory));
   setenv("XDG_CONFIG_HOME", directory, 1);
   Equation_Config config, loaded;
   equation_config_load(&config);
   EQUATION_CHECK(!strcmp(config.theme, "observatory") && config.mode == 0 && !config.degrees
                  && config.animations);
   EQUATION_CHECK(!ecore_file_exists(config.path));
   snprintf(config.theme, sizeof(config.theme), "/tmp/Themes & 日本語/crazy lab.edj");
   config.mode = 1;
   config.degrees = true;
   config.animations = false;
   EQUATION_CHECK(equation_config_save(&config));
   equation_config_load(&loaded);
   EQUATION_CHECK(!strcmp(config.theme, loaded.theme) && loaded.mode == 1 && loaded.degrees
                  && !loaded.animations);
   Eet_File *file = eet_open(config.path, EET_FILE_MODE_WRITE);
   EQUATION_CHECK(file);
   EQUATION_CHECK(eet_write(file, "appearance/theme", "unterminated", 12, 1) > 0);
   EQUATION_CHECK(eet_write(file, "calculator/mode", "99", 3, 1) > 0);
   EQUATION_CHECK(eet_close(file) == EET_ERROR_NONE);
   equation_config_load(&loaded);
   EQUATION_CHECK(!strcmp(loaded.theme, "observatory") && !loaded.mode && !loaded.degrees);
   char oversized[PATH_MAX + 32];
   memset(oversized, 'x', sizeof(oversized));
   oversized[0] = '/';
   oversized[sizeof(oversized) - 1] = 0;
   setenv("XDG_CONFIG_HOME", oversized, 1);
   equation_config_load(&loaded);
   EQUATION_CHECK(!loaded.path[0] && !equation_config_save(&loaded));
   EQUATION_CHECK(ecore_file_recursive_rm(directory));
   ecore_file_shutdown();
   eet_shutdown();
   return 0;
}
