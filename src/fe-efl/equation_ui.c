/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) Enlightenment Development Team
 */

#include "equation_ui.h"
#include "equation_intl.h"
#include "equation_engine.h"
#include "equation_config.h"
#include "equation_theme.h"
#include "equation_window.h"
#include <Elementary.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "equation_ui_private.h"

static void
equation_window_focus(void *data, Evas_Object *object EINA_UNUSED, void *event EINA_UNUSED)
{
   Equation_Ui *ui = data;
   if (!ui->menu && !elm_object_focus_get(ui->entry)) elm_object_focus_set(ui->entry, EINA_TRUE);
}

static Evas_Object *
equation_label(Equation_Ui *ui, Evas_Object *parent, const char *text)
{
   Evas_Object *object = elm_label_add(parent);
   elm_object_theme_set(object, ui->theme);
   elm_layout_theme_set(object, "label", "base", "default");
   elm_object_text_set(object, text);
   evas_object_size_hint_weight_set(object, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(object, 0, 0.5);
   evas_object_show(object);
   return object;
}

static void
equation_result_set(Evas_Object *entry, const char *text)
{
   char *markup = elm_entry_utf8_to_markup(text);
   elm_entry_entry_set(entry, markup ? markup : "");
   free(markup);
}

static void
equation_menu_close(void *data, Evas_Object *object EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Equation_Ui *ui = data;
   Evas_Object *menu = ui->menu;
   ui->menu = ui->theme_status = NULL;
   if (menu) evas_object_del(menu);
   if (ui->ready) elm_object_focus_set(ui->entry, EINA_TRUE);
}

static Evas_Object *
equation_menu_button(Evas_Object *box, const char *text, Evas_Smart_Cb callback, Equation_Ui *ui)
{
   Evas_Object *button = elm_button_add(box);
   elm_object_text_set(button, text);
   evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(button, EVAS_HINT_FILL, 0.5);
   evas_object_smart_callback_add(button, "clicked", callback, ui);
   elm_box_pack_end(box, button);
   evas_object_show(button);
   return button;
}

static void
equation_about_deleted(void *data, Evas *canvas EINA_UNUSED, Evas_Object *object EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   ((Equation_Ui *)data)->about = NULL;
}

static void
equation_about_close(void *data, Evas_Object *object EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Equation_Ui *ui = data;
   if (ui->about) evas_object_del(ui->about);
}

static void
equation_about_show(void *data, Evas_Object *object EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Equation_Ui *ui = data;
   equation_menu_close(ui, NULL, NULL);
   if (ui->about)
     {
        elm_win_activate(ui->about);
        return;
     }
   ui->about = equation_window_add(ui->window, _("About Equation"), ui->theme_path, ui->theme);
   if (!ui->about) return;
   elm_win_title_set(ui->about, _("About Equation"));
   elm_win_autodel_set(ui->about, EINA_TRUE);
   elm_object_theme_set(ui->about, ui->theme);
   evas_object_event_callback_add(ui->about, EVAS_CALLBACK_DEL, equation_about_deleted, ui);
   Evas_Object *box = elm_box_add(ui->about);
   elm_box_padding_set(box, 12, 12);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   equation_window_content_set(ui->about, box);
   elm_box_pack_end(box, equation_label(ui, box, "<b>Equation " EQUATION_VERSION "</b>"));
   char *about_text = elm_entry_utf8_to_markup(
     _("Scientific and programmer calculator\nDecimal, hexadecimal and binary results\n\nBuilt with "
       "Elementary / EFL\n\n(c) enlightenment development team\nGNU General Public License, version "
       "2"));
   elm_box_pack_end(box, equation_label(ui, box, about_text));
   free(about_text);
   equation_menu_button(box, _("Close"), equation_about_close, ui);
   evas_object_show(box);
   equation_window_size_minimum(ui->about);
   evas_object_show(ui->about);
}

static void
equation_quit(void *data, Evas_Object *object EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Equation_Ui *ui = data;
   evas_object_del(ui->window);
   elm_exit();
}

static void
equation_theme_choices_update(Equation_Ui *ui)
{
   if (!ui->menu) return;
   Evas_Object *box = elm_object_content_get(ui->menu), *button;
   Eina_List *items = elm_box_children_get(box), *node;
   EINA_LIST_FOREACH(items, node, button)
   {
      const char *name = evas_object_data_get(button, "equation_theme_name");
      if (name)
        elm_object_signal_emit(
          button, !strcmp(name, ui->config.theme) ? "equation,selected,on" : "equation,selected,off",
          "equation");
   }
   eina_list_free(items);
}

static void
equation_preferences_save(Equation_Ui *ui)
{
   if (!ui->ready) return;
   ui->config_failed = !equation_config_save(&ui->config);
   if (ui->theme_status)
     {
        Evas_Object *box = elm_object_parent_widget_get(ui->theme_status);
        elm_box_unpack(box, ui->theme_status);
        elm_object_text_set(ui->theme_status,
                         ui->config_failed ? _("Applied, but preferences could not be saved.") : "");
        if (ui->config_failed)
          {
             elm_box_pack_end(box, ui->theme_status);
             evas_object_show(ui->theme_status);
          }
        else
          evas_object_hide(ui->theme_status);
     }
}

static void
equation_motion_sync(Equation_Ui *ui)
{
   bool running = ui->config.animations && evas_object_visible_get(ui->window)
                  && !elm_win_iconified_get(ui->window);
   if (ui->motion_running == running) return;
   ui->motion_running = running;
   elm_layout_signal_emit(ui->display, running ? "equation,motion,on" : "equation,motion,off",
                          "equation");
}

static void
equation_motion_visibility(void *data, Evas *canvas EINA_UNUSED, Evas_Object *object EINA_UNUSED,
                           void *event EINA_UNUSED)
{
   equation_motion_sync(data);
}

static void
equation_motion_window_state(void *data, Evas_Object *object EINA_UNUSED, void *event EINA_UNUSED)
{
   equation_motion_sync(data);
}

static void
equation_motion_changed(void *data, Evas_Object *object, void *event EINA_UNUSED)
{
   Equation_Ui *ui = data;
   ui->config.animations = elm_check_state_get(object);
   equation_motion_sync(ui);
   equation_preferences_save(ui);
}

static void
equation_display_note(Equation_Ui *ui)
{
   const char *note = ui->copy_success     ? _("COPIED TO CLIPBOARD")
                      : ui->config.mode    ? _("PROGRAMMER / UNSIGNED 64")
                      : ui->config.degrees ? _("SCIENTIFIC / DEGREES")
                                           : _("SCIENTIFIC / RADIANS");
   elm_object_part_text_set(ui->display, "equation.note", note);
}

static Eina_Bool
equation_copy_reset(void *data)
{
   Equation_Ui *ui = data;
   ui->copy_timer = NULL;
   ui->copy_success = false;
   elm_image_file_set(ui->copy_icon, ui->theme_path, "equation/icon/copy");
   elm_object_tooltip_text_set(ui->copy, _("Copy result"));
   equation_display_note(ui);
   return ECORE_CALLBACK_CANCEL;
}

static void
equation_copy_clicked(void *data, Evas_Object *object EINA_UNUSED, void *event EINA_UNUSED)
{
   Equation_Ui *ui = data;
   if (!ui->result_valid) return;
   if (ui->copy_timer)
     {
        ecore_timer_del(ui->copy_timer);
        ui->copy_timer = NULL;
     }
   ui->copy_success = elm_cnp_selection_set(ui->window, ELM_SEL_TYPE_CLIPBOARD, ELM_SEL_FORMAT_TEXT,
                                            ui->display_value, strlen(ui->display_value));
   elm_image_file_set(ui->copy_icon, ui->theme_path,
                      ui->copy_success ? "equation/icon/check" : "equation/icon/copy");
   elm_object_tooltip_text_set(ui->copy,
                               ui->copy_success ? _("Copied") : _("Clipboard unavailable"));
   equation_display_note(ui);
   ui->copy_timer = ecore_timer_add(1.6, equation_copy_reset, ui);
}

static void
equation_result_rows_theme(Equation_Ui *ui)
{
   elm_object_part_text_set(ui->display, "caption", _("DECIMAL / RESULT"));
   static const char *const labels[] = { N_("HEX / 16"), N_("BIN / 02") };
   for (unsigned i = 0; i < 2; ++i)
     {
        elm_object_part_content_unset(ui->result_rows[i], "equation.result");
        elm_layout_file_set(ui->result_rows[i], ui->theme_path, "equation/result-row");
        elm_object_part_text_set(ui->result_rows[i], "equation.label", _(labels[i]));
        elm_object_part_content_set(ui->result_rows[i], "equation.result", ui->results[i]);
     }
}

static void
equation_theme_apply(Equation_Ui *ui, const char *name)
{
   char path[PATH_MAX];
   if (!equation_theme_resolve(name, path, sizeof(path))) return;
   if (strcmp(path, ui->theme_path))
     {
        elm_theme_overlay_add(ui->theme, path);
        elm_theme_overlay_del(ui->theme, ui->theme_path);
        snprintf(ui->theme_path, sizeof(ui->theme_path), "%s", path);
     }
   snprintf(ui->config.theme, sizeof(ui->config.theme), "%s", name);
   equation_window_theme_set(ui->window, path);
   if (ui->about) equation_window_theme_set(ui->about, path);
   elm_object_part_content_unset(ui->display, "equation.copy");
   elm_layout_file_set(ui->display, path, "equation/display");
   elm_object_part_content_set(ui->display, "equation.copy", ui->copy);
   if (ui->copy_timer)
     {
        ecore_timer_del(ui->copy_timer);
        ui->copy_timer = NULL;
     }
   equation_copy_reset(ui);
   equation_result_rows_theme(ui);
   elm_layout_signal_emit(ui->display, "equation,motion,off", "equation");
   ui->motion_running = false;
   equation_motion_sync(ui);
   equation_theme_display_set(ui->display, ui->display_value);
   equation_preferences_save(ui);
   equation_theme_choices_update(ui);
}

static void
equation_theme_selected(void *data, Evas_Object *object, void *event_info EINA_UNUSED)
{
   Equation_Ui *ui = data;
   const char *name = evas_object_data_get(object, "equation_theme_name");
   equation_theme_apply(ui, name);
}

static void
equation_menu_show(void *data, Evas_Object *object EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Equation_Ui *ui = data;
   if (ui->menu)
     {
        evas_object_show(ui->menu);
        return;
     }
   ui->menu = elm_popup_add(ui->window);
   elm_object_part_text_set(ui->menu, "title,text", "Equation");
   evas_object_smart_callback_add(ui->menu, "block,clicked", equation_menu_close, ui);
   Evas_Object *box = elm_box_add(ui->menu);
   elm_box_padding_set(box, 0, 6);
   evas_object_size_hint_min_set(box, 280, 0);
   elm_box_pack_end(box, equation_label(ui, box, _("<b>Appearance</b>")));
   for (unsigned i = 0; i < EINA_C_ARRAY_LENGTH(equation_theme_catalog); ++i)
     {
        Evas_Object *button = equation_menu_button(box, _(equation_theme_catalog[i].label),
                                                   equation_theme_selected, ui);
        evas_object_data_set(button, "equation_theme_name", equation_theme_catalog[i].name);
        evas_object_name_set(button, equation_theme_catalog[i].name);
        elm_object_tooltip_text_set(button, _(equation_theme_catalog[i].description));
     }
   Evas_Object *animations = elm_check_add(box);
   elm_object_text_set(animations, _("Ambient animations"));
   elm_check_state_set(animations, ui->config.animations);
   evas_object_name_set(animations, "equation_animations");
   evas_object_smart_callback_add(animations, "changed", equation_motion_changed, ui);
   elm_box_pack_end(box, animations);
   evas_object_show(animations);
   ui->theme_status
     = equation_label(ui, box, ui->config_failed ? _("Preferences could not be saved.") : "");
   if (ui->config_failed)
     elm_box_pack_end(box, ui->theme_status);
   else
     evas_object_hide(ui->theme_status);
   evas_object_name_set(equation_menu_button(box, _("About Equation"), equation_about_show, ui),
                        "equation_about");
   evas_object_name_set(equation_menu_button(box, _("Quit"), equation_quit, ui), "equation_quit");
   elm_object_content_set(ui->menu, box);
   evas_object_show(box);
   Evas_Object *close = elm_button_add(ui->menu);
   elm_object_text_set(close, _("Close"));
   evas_object_smart_callback_add(close, "clicked", equation_menu_close, ui);
   elm_object_part_content_set(ui->menu, "button1", close);
   evas_object_show(ui->menu);
   equation_theme_choices_update(ui);
}

static void
equation_input_guide_update(Equation_Ui *ui)
{
   const int base = elm_radio_value_get(ui->mode) == 1 ? elm_radio_value_get(ui->input_base) : 10;
   elm_object_part_text_set(ui->entry, "guide",
                            base == 16  ? _("Hex expression (e.g. FF + 1)")
                            : base == 2 ? _("Binary expression (e.g. 1010 + 1)")
                                        : _("Decimal expression (e.g. 42 + 1)"));
}

static void
equation_evaluate_cb(void *data, Evas_Object *object EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Equation_Ui *ui = data;
   char *input = elm_entry_markup_to_utf8(elm_entry_entry_get(ui->entry));
   Equation_Result result = equation_evaluate_base(
     input ? input : "",
     elm_radio_value_get(ui->mode) == 1 ? EQUATION_MODE_PROGRAMMER : EQUATION_MODE_SCIENTIFIC,
     elm_check_state_get(ui->degrees),
     elm_radio_value_get(ui->mode) == 1 ? elm_radio_value_get(ui->input_base) : 10);
   free(input);
   snprintf(ui->display_value, sizeof(ui->display_value), "%s", result.valid ? result.decimal : "—");
   equation_theme_display_set(ui->display, ui->display_value);
   ui->result_valid = result.valid;
   elm_object_disabled_set(ui->copy, !result.valid);
   if (ui->copy_timer)
     {
        ecore_timer_del(ui->copy_timer);
        ui->copy_timer = NULL;
     }
   equation_copy_reset(ui);
   equation_display_note(ui);
   if (ui->motion_running && result.valid)
     elm_layout_signal_emit(ui->display, "equation,result", "equation");
   elm_box_unpack(ui->box, ui->status);
   if (!result.valid)
     {
        char *error = elm_entry_utf8_to_markup(result.error);
        elm_object_text_set(ui->status, error);
        free(error);
        elm_box_pack_after(ui->box, ui->status, ui->display);
        evas_object_show(ui->status);
     }
   else
     evas_object_hide(ui->status);
   equation_result_set(ui->results[0], result.valid ? result.hexadecimal : "—");
   equation_result_set(ui->results[1], result.valid ? result.binary : "—");
}

static void
equation_mode_cb(void *data, Evas_Object *object EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Equation_Ui *ui = data;
   bool programmer = elm_radio_value_get(ui->mode) == 1;
   elm_box_unpack(ui->function_box, ui->scientific);
   elm_box_unpack(ui->function_box, ui->programmer);
   elm_box_pack_end(ui->function_box, programmer ? ui->programmer : ui->scientific);
   if (programmer)
     evas_object_hide(ui->scientific);
   else
     evas_object_show(ui->scientific);
   if (programmer)
     evas_object_show(ui->programmer);
   else
     evas_object_hide(ui->programmer);
   if (programmer)
     evas_object_hide(ui->degrees);
   else
     evas_object_show(ui->degrees);
   if (programmer)
     {
        elm_box_pack_after(ui->box, ui->base_box, ui->entry);
        evas_object_show(ui->base_box);
     }
   else
     {
        elm_box_unpack(ui->box, ui->base_box);
        evas_object_hide(ui->base_box);
     }
   equation_input_guide_update(ui);
   equation_evaluate_cb(ui, NULL, NULL);
   ui->config.mode = programmer;
   equation_display_note(ui);
   equation_preferences_save(ui);
   if (ui->ready) equation_window_size_minimum(ui->window);
}

static void
equation_base_cb(void *data, Evas_Object *object EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Equation_Ui *ui = data;
   equation_input_guide_update(ui);
   equation_evaluate_cb(ui, NULL, NULL);
   elm_object_focus_set(ui->entry, EINA_TRUE);
}

static void
equation_degrees_cb(void *data, Evas_Object *object EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Equation_Ui *ui = data;
   ui->config.degrees = elm_check_state_get(ui->degrees);
   equation_evaluate_cb(ui, NULL, NULL);
   equation_preferences_save(ui);
}

static void
equation_button_cb(void *data, Evas_Object *button, void *event_info EINA_UNUSED)
{
   Equation_Ui *ui = data;
   const char *token = evas_object_data_get(button, "equation_token");

   if (!strcmp(token, "="))
     {
        equation_evaluate_cb(ui, NULL, NULL);
        return;
     }
   if (!strcmp(token, "Clear"))
     elm_entry_entry_set(ui->entry, "");
   else if (!strcmp(token, "Back"))
     {
        if (elm_entry_selection_get(ui->entry))
          elm_entry_entry_insert(ui->entry, "");
        else
          {
             elm_entry_select_none(ui->entry);
             elm_entry_select_allow_set(ui->entry, EINA_TRUE);
             int cursor = elm_entry_cursor_pos_get(ui->entry);
             if (cursor > 0)
               {
                  elm_entry_select_region_set(ui->entry, cursor - 1, cursor);
                  elm_entry_entry_insert(ui->entry, "");
               }
          }
     }
   else
     {
        char *markup = elm_entry_utf8_to_markup(token);
        elm_entry_entry_insert(ui->entry, markup);
        free(markup);
     }
   elm_object_focus_set(ui->entry, EINA_TRUE);
}

static Evas_Object *
equation_keypad(Equation_Ui *ui, Evas_Object *parent, const char *const *keys, size_t count,
                int columns, bool advanced)
{
   Evas_Object *table = elm_table_add(parent);
   elm_table_padding_set(table, ELM_SCALE_SIZE(3), ELM_SCALE_SIZE(3));
   elm_table_homogeneous_set(table, EINA_TRUE);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
   for (size_t i = 0; i < count; ++i)
     {
        Evas_Object *button = elm_button_add(table);
        elm_object_focus_allow_set(button, EINA_FALSE);
        char *markup = elm_entry_utf8_to_markup(!strcmp(keys[i], "Clear")  ? _("Clear")
                                                : !strcmp(keys[i], "Back") ? _("Back")
                                                                           : keys[i]);
        elm_object_text_set(button, markup);
        const char *style = !strcmp(keys[i], "=") ? "equation/equals"
                            : advanced            ? "equation/function"
                            : (strlen(keys[i]) == 1 && strchr("+-*/()", keys[i][0]))
                              ? "equation/operator"
                              : "default";
        elm_object_style_set(button, style);
        evas_object_data_set(button, "equation_key", keys[i]);
        free(markup);
        evas_object_data_set(button, "equation_token", keys[i]);
        evas_object_smart_callback_add(button, "clicked", equation_button_cb, ui);
        evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_table_pack(table, button, i % columns, i / columns, 1, 1);
        evas_object_show(button);
     }
   elm_box_pack_end(parent, table);
   evas_object_show(table);
   return table;
}

static void
equation_ui_free(void *data, Evas *canvas EINA_UNUSED, Evas_Object *object EINA_UNUSED,
                 void *event_info EINA_UNUSED)
{
   Equation_Ui *ui = data;
   ui->ready = false;
   equation_menu_close(ui, NULL, NULL);
   if (ui->copy_timer) ecore_timer_del(ui->copy_timer);
   if (ui->about) evas_object_del(ui->about);

   elm_theme_overlay_del(ui->theme, ui->theme_path);
   free(ui);
}

Evas_Object *
equation_ui_show(void)
{
   Equation_Ui *ui = calloc(1, sizeof(*ui));
   if (!ui) return NULL;
   equation_config_load(&ui->config);
   if (!equation_theme_resolve(ui->config.theme, ui->theme_path, sizeof(ui->theme_path)))
     {
        snprintf(ui->config.theme, sizeof(ui->config.theme), "observatory");
        if (!equation_theme_resolve(ui->config.theme, ui->theme_path, sizeof(ui->theme_path)))
          {
             fprintf(stderr, "Equation: cannot load its installed theme archives.\n");
             free(ui);
             return NULL;
          }
     }
   ui->theme = elm_theme_default_get();
   elm_theme_overlay_add(ui->theme, ui->theme_path);
   ui->window = equation_window_add(NULL, "Equation", ui->theme_path, ui->theme);
   if (!ui->window)
     {
        elm_theme_overlay_del(ui->theme, ui->theme_path);
        free(ui);
        return NULL;
     }
   equation_window_menu_set(ui->window, equation_menu_show, ui);
   elm_win_autodel_set(ui->window, EINA_TRUE);
   evas_object_data_set(ui->window, "equation_ui", ui);
   evas_object_event_callback_add(ui->window, EVAS_CALLBACK_DEL, equation_ui_free, ui);
   Evas_Object *box = ui->box = elm_box_add(ui->window);
   elm_box_padding_set(box, 0, 5);
   elm_box_align_set(box, 0.5, 0);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   equation_window_content_set(ui->window, box);
   evas_object_show(box);

   Evas_Object *top = elm_box_add(box);
   elm_box_horizontal_set(top, EINA_TRUE);
   elm_box_pack_end(box, top);
   evas_object_show(top);
   ui->mode = elm_radio_add(top);
   elm_object_focus_allow_set(ui->mode, EINA_FALSE);
   evas_object_name_set(ui->mode, "equation_scientific");
   elm_object_text_set(ui->mode, _("Scientific"));
   elm_radio_state_value_set(ui->mode, 0);
   elm_box_pack_end(top, ui->mode);
   evas_object_show(ui->mode);
   Evas_Object *programmer = elm_radio_add(top);
   elm_object_focus_allow_set(programmer, EINA_FALSE);
   evas_object_name_set(programmer, "equation_programmer");
   elm_object_text_set(programmer, _("Programmer"));
   elm_radio_state_value_set(programmer, 1);
   elm_radio_group_add(programmer, ui->mode);
   elm_box_pack_end(top, programmer);
   evas_object_show(programmer);
   evas_object_smart_callback_add(ui->mode, "changed", equation_mode_cb, ui);
   evas_object_smart_callback_add(programmer, "changed", equation_mode_cb, ui);
   elm_radio_value_set(ui->mode, ui->config.mode);
   ui->degrees = elm_check_add(top);
   elm_object_focus_allow_set(ui->degrees, EINA_FALSE);
   elm_check_state_set(ui->degrees, ui->config.degrees);
   elm_object_text_set(ui->degrees, _("Degrees"));
   elm_object_tooltip_text_set(ui->degrees, _("Unchecked: angles in radians"));
   elm_box_pack_end(top, ui->degrees);
   evas_object_show(ui->degrees);
   evas_object_smart_callback_add(ui->degrees, "changed", equation_degrees_cb, ui);

   ui->entry = elm_entry_add(box);
   elm_entry_single_line_set(ui->entry, EINA_TRUE);
   elm_entry_scrollable_set(ui->entry, EINA_TRUE);
   elm_object_part_text_set(ui->entry, "guide", _("Enter an expression, then press Enter"));
   evas_object_size_hint_weight_set(ui->entry, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(ui->entry, EVAS_HINT_FILL, 0.5);
   elm_box_pack_end(box, ui->entry);
   evas_object_show(ui->entry);
   evas_object_smart_callback_add(ui->entry, "activated", equation_evaluate_cb, ui);
   elm_entry_cnp_mode_set(ui->entry, ELM_CNP_MODE_PLAINTEXT);
   ui->base_box = elm_box_add(box);
   elm_box_horizontal_set(ui->base_box, EINA_TRUE);
   elm_box_padding_set(ui->base_box, 8, 0);
   elm_box_pack_end(ui->base_box, equation_label(ui, ui->base_box, _("Input base:")));
   const char *base_labels[] = { _("Decimal (10)"), _("Hex (16)"), _("Binary (2)") };
   const char *base_names[]
     = { "equation_base_decimal", "equation_base_hex", "equation_base_binary" };
   const int bases[] = { 10, 16, 2 };
   for (unsigned i = 0; i < EINA_C_ARRAY_LENGTH(bases); ++i)
     {
        Evas_Object *radio = elm_radio_add(ui->base_box);
        elm_object_focus_allow_set(radio, EINA_FALSE);
        elm_object_text_set(radio, base_labels[i]);
        evas_object_name_set(radio, base_names[i]);
        elm_radio_state_value_set(radio, bases[i]);
        if (i)
          elm_radio_group_add(radio, ui->input_base);
        else
          ui->input_base = radio;
        evas_object_smart_callback_add(radio, "changed", equation_base_cb, ui);
        elm_box_pack_end(ui->base_box, radio);
        evas_object_show(radio);
     }
   elm_radio_value_set(ui->input_base, 10);
   elm_object_tooltip_text_set(ui->base_box,
                               _("Base for unprefixed integers. Use 0b, 0d or 0x to override it."));
   ui->display = elm_layout_add(box);
   elm_layout_file_set(ui->display, ui->theme_path, "equation/display");
   evas_object_size_hint_weight_set(ui->display, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(ui->display, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_min_set(ui->display, 0, ELM_SCALE_SIZE(108));
   elm_box_pack_end(box, ui->display);
   evas_object_show(ui->display);
   ui->copy = elm_button_add(ui->display);
   elm_object_focus_allow_set(ui->copy, EINA_FALSE);
   elm_object_style_set(ui->copy, "equation/copy");
   evas_object_name_set(ui->copy, "equation_copy");
   elm_object_access_info_set(ui->copy, _("Copy result"));
   elm_object_tooltip_text_set(ui->copy, _("Copy result"));
   ui->copy_icon = elm_icon_add(ui->copy);
   elm_image_file_set(ui->copy_icon, ui->theme_path, "equation/icon/copy");
   evas_object_size_hint_min_set(ui->copy_icon, ELM_SCALE_SIZE(18), ELM_SCALE_SIZE(18));
   elm_object_content_set(ui->copy, ui->copy_icon);
   evas_object_smart_callback_add(ui->copy, "clicked", equation_copy_clicked, ui);
   elm_object_part_content_set(ui->display, "equation.copy", ui->copy);
   evas_object_show(ui->copy);
   ui->status = equation_label(ui, box, "");
   elm_box_pack_end(box, ui->status);
   for (size_t i = 0; i < 2; ++i)
     {
        Evas_Object *row = ui->result_rows[i] = elm_layout_add(box);
        elm_layout_file_set(row, ui->theme_path, "equation/result-row");
        evas_object_size_hint_weight_set(row, EVAS_HINT_EXPAND, 0);
        evas_object_size_hint_align_set(row, EVAS_HINT_FILL, 0.5);
        evas_object_size_hint_min_set(row, 0, ELM_SCALE_SIZE(30));
        Evas_Object *entry = ui->results[i] = elm_entry_add(row);
        elm_object_focus_allow_set(entry, EINA_FALSE);
        elm_object_style_set(entry, "equation/result");
        elm_entry_single_line_set(entry, EINA_TRUE);
        elm_entry_editable_set(entry, EINA_FALSE);
        elm_entry_scrollable_set(entry, EINA_TRUE);
        elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_PLAINTEXT);
        elm_object_access_info_set(entry, i == 0 ? _("Hexadecimal result") : _("Binary result"));
        evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_object_part_content_set(row, "equation.result", entry);
        elm_box_pack_end(box, row);
        evas_object_show(entry);
        evas_object_show(row);
     }
   equation_result_rows_theme(ui);
   Evas_Object *keypads = elm_box_add(box);
   elm_box_horizontal_set(keypads, EINA_TRUE);
   elm_box_padding_set(keypads, ELM_SCALE_SIZE(8), 0);
   evas_object_size_hint_weight_set(keypads, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(keypads, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(box, keypads);
   evas_object_show(keypads);
   ui->function_box = elm_box_add(keypads);
   evas_object_size_hint_weight_set(ui->function_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(ui->function_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(keypads, ui->function_box);
   evas_object_show(ui->function_box);
   static const char *const scientific[]
     = { "sin(", "cos(", "tan(", "sqrt(", "asin(", "acos(", "atan(", "abs(", "ln(",    "log(",
          "exp(", "^",    "pi",   "e",     ".",     "%",     "1/(",   "10^",  "floor(", "ceil(" };
   static const char *const programmer_keys[]
     = { "0x", "0b", "<<", ">>", "&", "|",  "^",    "~",      "A",   "B",
          "C",  "D",  "E",  "F",  "%", "00", "0xFF", "0xFFFF", "1<<", "~0" };
   ui->scientific = equation_keypad(ui, ui->function_box, scientific, 20, 4, true);
   ui->programmer = equation_keypad(ui, ui->function_box, programmer_keys, 20, 4, true);
   static const char *const common[] = { "Clear", "Back", "(", ")", "7", "8", "9", "/",  "4", "5",
                                          "6",     "*",    "1", "2", "3", "-", "0", "00", "=", "+" };
   ui->common = equation_keypad(ui, keypads, common, 20, 4, false);
   evas_object_size_hint_weight_set(ui->scientific, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_weight_set(ui->programmer, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_weight_set(ui->common, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   equation_result_set(ui->entry, "0");
   equation_mode_cb(ui, NULL, NULL);
   elm_theme_flush(ui->theme);
   ui->ready = true;
   equation_window_size_minimum(ui->window);
   evas_object_event_callback_add(ui->window, EVAS_CALLBACK_SHOW, equation_motion_visibility, ui);
   evas_object_event_callback_add(ui->window, EVAS_CALLBACK_HIDE, equation_motion_visibility, ui);
   evas_object_smart_callback_add(ui->window, "iconified", equation_motion_window_state, ui);
   evas_object_smart_callback_add(ui->window, "normal", equation_motion_window_state, ui);
   evas_object_show(ui->window);
   equation_motion_sync(ui);
   evas_object_smart_callback_add(ui->window, "focus,in", equation_window_focus, ui);
   elm_object_focus_set(ui->entry, EINA_TRUE);
   elm_entry_select_all(ui->entry);
   return ui->window;
}
