/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) Enlightenment Development Team
 */

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static Ecore_Evas *equation_canvas;

static void
equation_pump(void)
{
   for (int i = 0; i < 50; ++i)
     {
        ecore_main_loop_iterate_may_block(EINA_FALSE);
        if (!(i % 5)) ecore_evas_manual_render(equation_canvas);
        usleep(5000);
     }
}

static void
equation_orbit_position(Evas_Object *display, int *x, int *y)
{
   edje_object_calc_force(display);
   const Evas_Object *orbit = edje_object_part_object_get(display, "motion_orbit");
   const Evas_Map *map = evas_object_map_get(orbit);
   *x = *y = 0;
   if (map) evas_map_point_coord_get(map, 0, x, y, NULL);
}

int
main(int argc, char **argv)
{
   ecore_evas_init();
   edje_init();
   edje_transition_duration_factor_set(1.0);
   Ecore_Evas *canvas = ecore_evas_buffer_new(960, 1600);
   if (!canvas || argc < 2) return 1;
   equation_canvas = canvas;
   ecore_evas_show(canvas);
   int failed = 0;
   for (int i = 1; i < argc; ++i)
     {
        Eina_List *groups = edje_file_collection_list(argv[i]), *node;
        const char *group;
        if (!groups) failed++;
        const char *required[]
          = { "equation/frame",         "equation/display",       "elm/button/base/default",
               "elm/radio/base/default", "elm/label/base/default", "elm/popup/base/default" };
        for (unsigned j = 0; j < sizeof(required) / sizeof(*required); ++j)
          if (!edje_file_group_exists(argv[i], required[j]))
            {
               fprintf(stderr, "Missing %s\n", required[j]);
               failed++;
            }
        EINA_LIST_FOREACH(groups, node, group)
        {
           Evas_Object *object = edje_object_add(ecore_evas_get(canvas));
           if (!edje_object_file_set(object, argv[i], group))
             {
                fprintf(stderr, "%s: %s failed\n", argv[i], group);
                failed++;
             }
           for (int scale = 1; scale <= 2; ++scale)
             {
                edje_object_scale_set(object, scale);
                evas_object_resize(object, 480 * scale, 790 * scale);
                edje_object_calc_force(object);
                if (!strcmp(group, "equation/frame"))
                  {
                     const char *parts[] = { "equation.content",
                                              "equation.controls",
                                              "equation.menu",
                                              "equation.move",
                                              "top",
                                              "bottom",
                                              "left",
                                              "right",
                                              "tl",
                                              "tr",
                                              "bl",
                                             "br" };
                     for (unsigned k = 0; k < sizeof(parts) / sizeof(*parts); ++k)
                       {
                          int x, y, w, h;
                          if (!edje_object_part_geometry_get(object, parts[k], &x, &y, &w, &h) || w <= 0
                              || h <= 0 || x < 0 || y < 0 || x + w > 480 * scale || y + h > 790 * scale)
                            {
                               fprintf(stderr, "Invalid geometry %s %s at %d\n", argv[i], parts[k], scale);
                               failed++;
                            }
                       }
                     int bx, by, bw, bh, mx, my, mw, mh, tx, ty, tw, th;
                     edje_object_part_geometry_get(object, "body", &bx, &by, &bw, &bh);
                     edje_object_part_geometry_get(object, "equation.menu", &mx, &my, &mw, &mh);
                     edje_object_part_geometry_get(object, "equation.title", &tx, &ty, &tw, &th);
                     if (bx <= 0 || by <= 0 || mx < bx || mx + mw > tx || my < by || my + mh > by + bh)
                       {
                          fprintf(stderr, "Invalid shadow inset or menu/title order: %s at %d\n", argv[i],
                                  scale);
                          failed++;
                       }
                     edje_object_signal_emit(object, "equation,maximized", "equation");
                     edje_object_message_signal_process(object);
                     edje_object_calc_force(object);
                     edje_object_part_geometry_get(object, "body", &bx, &by, &bw, &bh);
                     double value;
                     const char *state = edje_object_part_state_get(object, "equation.shadow", &value);
                     if (bx || by || bw != 480 * scale || bh != 790 * scale || !state
                         || strcmp(state, "maximized"))
                       {
                          fprintf(stderr, "Maximized frame retains padding/shadow: %s\n", argv[i]);
                          failed++;
                       }
                     edje_object_signal_emit(object, "equation,restored", "equation");
                     edje_object_message_signal_process(object);
                     edje_object_calc_force(object);
                     edje_object_part_geometry_get(object, "body", &bx, &by, &bw, &bh);
                     if (bx <= 0 || by <= 0)
                       {
                          fprintf(stderr, "Restored shadow margin missing\n");
                          failed++;
                       }
                  }
             }
           if (!strcmp(group, "equation/display") && edje_object_part_exists(object, "tube_0"))
             {
                const int sizes[][2] = { { 360, 108 }, { 660, 108 }, { 1200, 108 }, { 660, 142 }, { 660, 300 } };
                for (int scale = 1; scale <= 2; ++scale)
                  {
                     edje_object_scale_set(object, scale);
                     for (unsigned size = 0; size < sizeof(sizes) / sizeof(*sizes); ++size)
                       {
                          evas_object_resize(object, sizes[size][0] * scale, sizes[size][1] * scale);
                          edje_object_calc_force(object);
                          int ax, ay, aw, ah;
                          edje_object_part_geometry_get(object, "tubes_area", &ax, &ay, &aw, &ah);
                          for (int digit = 0; digit < 14; ++digit)
                            {
                               char part[32];
                               snprintf(part, sizeof(part), "tube_%d", digit);
                               int x, y, w, h;
                               edje_object_part_geometry_get(object, part, &x, &y, &w, &h);
                               double error = w - .45 * h;
                               if (w <= 0 || h <= 0 || error < -1.5 || error > 1.5 || x < ax || y < ay
                                   || x + w > ax + aw || y + h > ay + ah)
                                 {
                                    fprintf(stderr, "Distorted/clipped %s at %dx%d scale %d\n", part,
                                            sizes[size][0], sizes[size][1], scale);
                                    failed++;
                                 }
                            }
                       }
                  }
             }
           if ((!strcmp(group, "elm/button/base/default")
                || !strcmp(group, "elm/button/base/equation/window")
                || !strcmp(group, "elm/button/base/equation/equals"))
               && edje_object_part_exists(object, "equation.hover_pulse"))
             {
                evas_object_show(object);
                equation_pump();
                edje_object_signal_emit(object, "mouse,in", "event");
                edje_object_message_signal_process(object);
                equation_pump();
                const Evas_Object *pulse = edje_object_part_object_get(object, "equation.hover_pulse");
                int first, second;
                edje_object_calc_force(object);
                evas_object_color_get(pulse, NULL, NULL, NULL, &first);
                equation_pump();
                edje_object_calc_force(object);
                evas_object_color_get(pulse, NULL, NULL, NULL, &second);
                int low = first < second ? first : second, high = first > second ? first : second;
                for (int sample = 0; sample < 5; ++sample)
                  {
                     equation_pump();
                     edje_object_calc_force(object);
                     evas_object_color_get(pulse, NULL, NULL, NULL, &second);
                     if (second < low) low = second;
                     if (second > high) high = second;
                  }
                if (low == high || high == 0)
                  {
                     fprintf(stderr, "Hover pulse is static: %s (%d, %d)\n", group, low, high);
                     failed++;
                  }
                edje_object_signal_emit(object, "mouse,out", "event");
                edje_object_message_signal_process(object);
                equation_pump();
                edje_object_calc_force(object);
                evas_object_color_get(pulse, NULL, NULL, NULL, &first);
                equation_pump();
                edje_object_calc_force(object);
                evas_object_color_get(pulse, NULL, NULL, NULL, &second);
                if (first || second)
                  {
                     fprintf(stderr, "Hover pulse did not stop: %s\n", group);
                     failed++;
                  }
             }
           if (!strcmp(group, "equation/display"))
             {
                evas_object_show(object);
                edje_object_part_text_set(object, "equation.value", "42");
                int x0, y0, x1, y1;
                equation_orbit_position(object, &x0, &y0);
                edje_object_signal_emit(object, "equation,motion,on", "equation");
                edje_object_message_signal_process(object);
                equation_pump();
                equation_orbit_position(object, &x1, &y1);
                if (x0 == x1 && y0 == y1)
                  {
                     fprintf(stderr, "Instrument did not animate: %s\n", argv[i]);
                     failed++;
                  }
                edje_object_signal_emit(object, "equation,motion,off", "equation");
                edje_object_message_signal_process(object);
                equation_pump();
                equation_orbit_position(object, &x0, &y0);
                equation_pump();
                equation_orbit_position(object, &x1, &y1);
                if (x0 != x1 || y0 != y1
                    || strcmp(edje_object_part_text_get(object, "equation.value"), "42"))
                  {
                     fprintf(stderr, "Motion did not stop or changed the value: %s\n", argv[i]);
                     failed++;
                  }
             }
           evas_object_del(object);
        }
        edje_file_collection_list_free(groups);
     }
   ecore_evas_free(canvas);
   edje_shutdown();
   ecore_evas_shutdown();
   return failed ? 1 : 0;
}
