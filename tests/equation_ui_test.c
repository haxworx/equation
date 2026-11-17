/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) Enlightenment Development Team
 */

#include "equation_ui.h"
#include "equation_intl.h"
#include "equation_ui_private.h"
#include <Ecore_File.h>
#include <Ecore_Evas.h>
#include <unistd.h>
#include <locale.h>
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

static void
equation_pump(void)
{
   for (int i = 0; i < 40; ++i)
     {
        ecore_main_loop_iterate_may_block(EINA_FALSE);
        usleep(5000);
     }
}

static void
equation_click(Evas_Object *object)
{
   int x, y, w, h;
   Evas *canvas = evas_object_evas_get(object);
   evas_object_geometry_get(object, &x, &y, &w, &h);
   evas_event_feed_mouse_move(canvas, x + w / 2, y + h / 2, 0, NULL);
   evas_event_feed_mouse_down(canvas, 1, EVAS_BUTTON_NONE, 0, NULL);
   evas_event_feed_mouse_up(canvas, 1, EVAS_BUTTON_NONE, 0, NULL);
   equation_pump();
}

static bool
equation_capture(Evas_Object *window, const char *path)
{
   Evas *canvas = evas_object_evas_get(window);
   Ecore_Evas *buffer = ecore_evas_ecore_evas_get(canvas);
   int w, h;
   ecore_evas_geometry_get(buffer, NULL, NULL, &w, &h);
   ecore_evas_manual_render(buffer);
   const void *pixels = ecore_evas_buffer_pixels_get(buffer);
   if (!pixels) return false;
   Evas_Object *image = evas_object_image_add(canvas);
   evas_object_image_size_set(image, w, h);
   evas_object_image_alpha_set(image, EINA_TRUE);
   evas_object_image_data_copy_set(image, (void *)pixels);
   bool result = evas_object_image_save(image, path, NULL, NULL);
   evas_object_del(image);
   return result;
}

typedef struct
{
   bool received;
   char text[128];
} Equation_Clipboard_Test;

static Eina_Bool
equation_clipboard_received(void *data, Evas_Object *object EINA_UNUSED,
                            Elm_Selection_Data *selection)
{
   Equation_Clipboard_Test *clipboard = data;
   size_t length
     = selection->len < sizeof(clipboard->text) - 1 ? selection->len : sizeof(clipboard->text) - 1;
   memcpy(clipboard->text, selection->data, length);
   clipboard->text[length] = 0;
   clipboard->received = true;
   return EINA_TRUE;
}

static void
equation_count(void *data, Evas_Object *object EINA_UNUSED, void *event EINA_UNUSED)
{
   ++*(int *)data;
}

static int
equation_button_interactions(Equation_Ui *ui)
{
   Evas_Object *key = elm_table_child_get(ui->common, 2, 1);
   Evas *canvas = evas_object_evas_get(key);
   Evas_Object *edje = elm_layout_edje_get(key);
   int x, y, w, h, clicks = 0, presses = 0;
   evas_object_geometry_get(key, &x, &y, &w, &h);
   evas_object_smart_callback_add(key, "clicked", equation_count, &clicks);
   evas_object_smart_callback_add(key, "pressed", equation_count, &presses);
   evas_event_feed_mouse_move(canvas, x + w / 2, y + h / 2, 0, NULL);
   equation_pump();
   const Evas_Button_Flags flags[]
     = { EVAS_BUTTON_NONE, EVAS_BUTTON_DOUBLE_CLICK, EVAS_BUTTON_TRIPLE_CLICK };
   for (unsigned i = 0; i < 3; ++i)
     {
        evas_event_feed_mouse_down(canvas, 1, flags[i], 0, NULL);
        equation_pump();
        EQUATION_CHECK(!strcmp(edje_object_part_state_get(edje, "surface", NULL), "pressed"));
        evas_event_feed_mouse_up(canvas, 1, flags[i], 0, NULL);
        equation_pump();
     }
   EQUATION_CHECK(clicks == 3 && presses == 3);
   /* Entry focus must not extinguish the key under the pointer. */
   elm_object_signal_emit(key, "elm,action,focus_highlight,show", "elm");
   elm_object_signal_emit(key, "elm,action,focus_highlight,hide", "elm");
   equation_pump();
   EQUATION_CHECK(!strcmp(edje_object_part_state_get(edje, "surface", NULL), "over"));
   /* Drag-out cancels activation and cannot leave a pressed/hovered key. */
   evas_event_feed_mouse_down(canvas, 1, EVAS_BUTTON_NONE, 0, NULL);
   evas_event_feed_mouse_move(canvas, 0, 0, 0, NULL);
   evas_event_feed_mouse_up(canvas, 1, EVAS_BUTTON_NONE, 0, NULL);
   equation_pump();
   EQUATION_CHECK(clicks == 3);
   EQUATION_CHECK(!strcmp(edje_object_part_state_get(edje, "surface", NULL), "default"));
   equation_click(key);
   EQUATION_CHECK(clicks == 4);
   elm_object_disabled_set(key, EINA_TRUE);
   equation_pump();
   equation_click(key);
   EQUATION_CHECK(clicks == 4);
   elm_object_disabled_set(key, EINA_FALSE);
   equation_pump();
   equation_click(key);
   EQUATION_CHECK(clicks == 5);
   evas_object_smart_callback_del_full(key, "clicked", equation_count, &clicks);
   evas_object_smart_callback_del_full(key, "pressed", equation_count, &presses);
   return 0;
}

int
main(int argc, char **argv)
{
   char directory[] = "/tmp/equation-ui-XXXXXX";
   EQUATION_CHECK(mkdtemp(directory));
   setenv("XDG_CONFIG_HOME", directory, 1);
   if (getenv("EQUATION_TEST_LOCALEDIR") && !setlocale(LC_ALL, "")) return 77;
   elm_init(argc, argv);
   equation_intl_init();
#if EQUATION_ENABLE_NLS
   const char *test_locale = getenv("EQUATION_TEST_LOCALEDIR");
   if (test_locale) bindtextdomain("equation", test_locale);
#endif
   Evas_Object *window = equation_ui_show();
   EQUATION_CHECK(window && elm_win_borderless_get(window));
   Equation_Ui *ui = evas_object_data_get(window, "equation_ui");
   EQUATION_CHECK(ui);
#if EQUATION_ENABLE_NLS
   if (test_locale)
     {
        EQUATION_CHECK(strcmp(_("Clear"), "Clear"));
        EQUATION_CHECK(
          !strcmp(elm_object_text_get(elm_table_child_get(ui->common, 0, 0)), _("Clear")));
        EQUATION_CHECK(!strcmp(elm_object_text_get(ui->mode), _("Scientific")));
     }
#endif
   Evas *canvas = evas_object_evas_get(window);
   const Evas_Object *window_icon = elm_win_icon_object_get(window);
   EQUATION_CHECK(window_icon != NULL);
   int icon_width, icon_height;
   elm_image_object_size_get(window_icon, &icon_width, &icon_height);
   EQUATION_CHECK(icon_width > 0 && icon_height > 0);
   equation_pump();
   /* Clicking other calculator controls must leave typing in the expression. */
   evas_focus_in(canvas);
   elm_object_focus_set(ui->entry, EINA_TRUE);
   equation_pump();
   equation_click(ui->degrees);
   EQUATION_CHECK(elm_object_focused_object_get(window) == ui->entry);
   elm_entry_entry_set(ui->entry, "");
   evas_event_feed_key_down(canvas, "4", "4", "4", "4", 0, NULL);
   evas_event_feed_key_up(canvas, "4", "4", "4", "4", 0, NULL);
   evas_event_feed_key_down(canvas, "2", "2", "2", "2", 0, NULL);
   evas_event_feed_key_up(canvas, "2", "2", "2", "2", 0, NULL);
   evas_event_feed_key_down(canvas, "Return", "Return", "\r", "\r", 0, NULL);
   evas_event_feed_key_up(canvas, "Return", "Return", "\r", "\r", 0, NULL);
   equation_pump();
   EQUATION_CHECK(!strcmp(elm_entry_entry_get(ui->entry), "42"));
   EQUATION_CHECK(!strcmp(ui->display_value, "42"));
   equation_click(ui->degrees);
   int width, height, min_width, min_height;
   evas_object_geometry_get(window, NULL, NULL, &width, &height);
   evas_object_size_hint_min_get(window, &min_width, &min_height);
   EQUATION_CHECK(min_width > 0 && min_height > 0 && width == min_width && height == min_height);
   for (unsigned i = 0; i < 3; ++i)
     {
        Evas_Object *menu_button = evas_object_name_find(canvas, "equation_menu");
        EQUATION_CHECK(menu_button);
        equation_click(menu_button);
        EQUATION_CHECK(ui->menu);
        const char *themes[] = { "observatory", "atelier", "crazy_lab" };
        Evas_Object *choice = evas_object_name_find(canvas, themes[i]);
        EQUATION_CHECK(choice);
        equation_click(choice);
        equation_pump();
        EQUATION_CHECK(!strcmp(ui->config.theme, themes[i]) && !ui->config_failed);
        Equation_Config loaded;
        equation_config_load(&loaded);
        EQUATION_CHECK(!strcmp(loaded.theme, themes[i]));
        Eina_List *children = elm_box_children_get(ui->box), *node;
        Evas_Object *child;
        EINA_LIST_FOREACH(children, node, child)
        {
           if (!strcmp(evas_object_type_get(child), "elm_label"))
             {
                const char *file, *group;
                edje_object_file_get(elm_layout_edje_get(child), &file, &group);
                EQUATION_CHECK(file && !strcmp(file, ui->theme_path));
             }
        }
        eina_list_free(children);
        elm_entry_entry_set(ui->entry, "sqrt(144)+30");
        evas_object_smart_callback_call(ui->entry, "activated", NULL);
        EQUATION_CHECK(!strcmp(ui->display_value, "42"));
        evas_object_smart_callback_call(ui->menu, "block,clicked", NULL);
        equation_pump();
        if (argc > 1)
          {
             char path[PATH_MAX];
             elm_entry_entry_set(ui->entry, "1234567890");
             evas_object_smart_callback_call(ui->entry, "activated", NULL);
             equation_pump();
             snprintf(path, sizeof(path), "%s/equation_%s.png", argv[1], themes[i]);
             EQUATION_CHECK(equation_capture(window, path));
          }
        EQUATION_CHECK(!equation_button_interactions(ui));
        /* The popup and About are both recreated and themed after each switch. */
        equation_click(menu_button);
        if (argc > 1)
          {
             char path[PATH_MAX];
             snprintf(path, sizeof(path), "%s/equation_%s_menu.png", argv[1], themes[i]);
             EQUATION_CHECK(equation_capture(window, path));
          }
        Evas_Object *about = evas_object_name_find(canvas, "equation_about");
        EQUATION_CHECK(about);
        equation_click(about);
        EQUATION_CHECK(!ui->menu && ui->about && elm_win_borderless_get(ui->about));
        equation_pump();
        if (argc > 1)
          {
             char path[PATH_MAX];
             snprintf(path, sizeof(path), "%s/equation_%s_about.png", argv[1], themes[i]);
             EQUATION_CHECK(equation_capture(ui->about, path));
          }
        /* Leave About open during the next switch, exercising live restyling. */
     }
   /* Exercise the actual keypad buttons after all theme changes. */
   const int keys[][2] = { { 0, 0 }, { 0, 2 }, { 3, 4 }, { 1, 3 }, { 2, 4 } };
   for (size_t i = 0; i < sizeof(keys) / sizeof(*keys); ++i)
     equation_click(elm_table_child_get(ui->common, keys[i][0], keys[i][1]));
   EQUATION_CHECK(!strcmp(ui->display_value, "6"));
   elm_entry_entry_set(ui->entry, "sqrt(144)+30");
   evas_object_smart_callback_call(ui->entry, "activated", NULL);
   equation_pump();
   EQUATION_CHECK(ui->result_valid && !elm_object_disabled_get(ui->copy));
   equation_click(ui->copy);
   EQUATION_CHECK(ui->copy_success);
   Equation_Clipboard_Test clipboard = { 0 };
   EQUATION_CHECK(elm_cnp_selection_get(window, ELM_SEL_TYPE_CLIPBOARD, ELM_SEL_FORMAT_TEXT,
                                        equation_clipboard_received, &clipboard));
   equation_pump();
   EQUATION_CHECK(clipboard.received && !strcmp(clipboard.text, "42"));
   elm_entry_entry_set(ui->entry, "1/0");
   evas_object_smart_callback_call(ui->entry, "activated", NULL);
   EQUATION_CHECK(!ui->result_valid && elm_object_disabled_get(ui->copy) && !ui->copy_success);
   elm_entry_entry_set(ui->entry, "42");
   evas_object_smart_callback_call(ui->entry, "activated", NULL);
   /* The appearance menu controls ambient animation. */
   Evas_Object *menu_button = evas_object_name_find(canvas, "equation_menu");
   equation_click(menu_button);
   Evas_Object *animations = evas_object_name_find(canvas, "equation_animations");
   EQUATION_CHECK(animations && ui->motion_running);
   equation_click(animations);
   EQUATION_CHECK(!ui->config.animations && !ui->motion_running);
   Equation_Config motion_config;
   equation_config_load(&motion_config);
   EQUATION_CHECK(!motion_config.animations);
   equation_click(animations);
   EQUATION_CHECK(ui->config.animations && ui->motion_running);
   /* A failed save does not prevent live theme changes or calculations. */
   char config_path[PATH_MAX];
   snprintf(config_path, sizeof(config_path), "%s", ui->config.path);
   snprintf(ui->config.path, sizeof(ui->config.path), "/dev/null/equation.eet");
   equation_click(evas_object_name_find(canvas, "atelier"));
   EQUATION_CHECK(ui->config_failed && !strcmp(ui->config.theme, "atelier"));
   EQUATION_CHECK(strstr(elm_object_text_get(ui->theme_status), "could not be saved"));
   snprintf(ui->config.path, sizeof(ui->config.path), "%s", config_path);
   equation_click(evas_object_name_find(canvas, "crazy_lab"));
   EQUATION_CHECK(!ui->config_failed);
   evas_object_smart_callback_call(ui->menu, "block,clicked", NULL);
   equation_click(evas_object_name_find(canvas, "equation_programmer"));
   EQUATION_CHECK(elm_radio_value_get(ui->mode) == 1 && ui->config.mode == 1);
   equation_click(ui->mode);
   EQUATION_CHECK(elm_radio_value_get(ui->mode) == 0 && ui->config.mode == 0);
   equation_click(ui->degrees);
   EQUATION_CHECK(elm_check_state_get(ui->degrees) && ui->config.degrees);
   equation_click(evas_object_name_find(canvas, "equation_programmer"));
   EQUATION_CHECK(evas_object_visible_get(ui->base_box));
   equation_click(evas_object_name_find(canvas, "equation_base_hex"));
   EQUATION_CHECK(elm_radio_value_get(ui->input_base) == 16);
   elm_entry_entry_set(ui->entry, "FF + 1");
   evas_object_smart_callback_call(ui->entry, "activated", NULL);
   EQUATION_CHECK(ui->result_valid && !strcmp(ui->display_value, "256"));
   equation_click(evas_object_name_find(canvas, "equation_base_binary"));
   EQUATION_CHECK(!ui->result_valid);
   elm_entry_entry_set(ui->entry, "1010 + 1");
   evas_object_smart_callback_call(ui->entry, "activated", NULL);
   EQUATION_CHECK(ui->result_valid && !strcmp(ui->display_value, "11"));
   equation_click(evas_object_name_find(canvas, "equation_base_decimal"));
   EQUATION_CHECK(ui->result_valid && !strcmp(ui->display_value, "1011"));
   equation_click(ui->mode);
   EQUATION_CHECK(!evas_object_visible_get(ui->base_box));
   elm_entry_entry_set(ui->entry, "sqrt(144)");
   evas_object_smart_callback_call(ui->entry, "activated", NULL);
   EQUATION_CHECK(ui->result_valid && !strcmp(ui->display_value, "12"));
   equation_click(evas_object_name_find(canvas, "equation_programmer"));
   elm_entry_entry_set(ui->entry, "0xffffffffffffffff");
   evas_object_smart_callback_call(ui->entry, "activated", NULL);
   EQUATION_CHECK(!strcmp(ui->display_value, "18446744073709551615"));
   equation_pump();
   equation_click(ui->copy);
   clipboard.received = false;
   EQUATION_CHECK(elm_cnp_selection_get(window, ELM_SEL_TYPE_CLIPBOARD, ELM_SEL_FORMAT_TEXT,
                                        equation_clipboard_received, &clipboard));
   equation_pump();
   EQUATION_CHECK(clipboard.received && !strcmp(clipboard.text, "18446744073709551615"));
   menu_button = evas_object_name_find(canvas, "equation_menu");
   equation_click(menu_button);
   Evas_Object *quit = evas_object_name_find(canvas, "equation_quit");
   EQUATION_CHECK(quit);
   equation_click(quit);
   window = equation_ui_show();
   EQUATION_CHECK(window);
   ui = evas_object_data_get(window, "equation_ui");
   EQUATION_CHECK(!strcmp(ui->config.theme, "crazy_lab") && ui->config.mode == 1
                  && ui->config.degrees);
   evas_object_del(window);
   elm_shutdown();
   EQUATION_CHECK(ecore_file_init());
   ecore_file_recursive_rm(directory);
   ecore_file_shutdown();
   return 0;
}
