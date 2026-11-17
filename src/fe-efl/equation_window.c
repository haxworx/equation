/* SPDX-License-Identifier: GPL-2.0-only
 * Copyright (c) Enlightenment Development Team
 */

#include "equation_window.h"
#include "equation_intl.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct
{
   Evas_Object *window, *frame, *controls, *content, *menu;
   const char *title;
   Eina_Bool dialog;
} Equation_Window;

static void
equation_window_close(void *data, Evas_Object *object EINA_UNUSED, void *event EINA_UNUSED)
{
   evas_object_del(data);
}

static void
equation_window_minimize(void *data, Evas_Object *object EINA_UNUSED, void *event EINA_UNUSED)
{
   elm_win_iconified_set(data, EINA_TRUE);
}

static void
equation_window_maximize(void *data, Evas_Object *object EINA_UNUSED, void *event EINA_UNUSED)
{
   elm_win_maximized_set(data, !elm_win_maximized_get(data));
}

static void
equation_window_move(void *data, Evas *canvas EINA_UNUSED, Evas_Object *object EINA_UNUSED,
                     void *event)
{
   Evas_Event_Mouse_Down *mouse = event;
   if (mouse->button != 1) return;
   if (mouse->flags & EVAS_BUTTON_DOUBLE_CLICK)
     equation_window_maximize(data, NULL, NULL);
   else
     elm_win_move_resize_start(data, ELM_WIN_MOVE_RESIZE_MOVE);
}

static void
equation_window_resize(void *data, Evas *canvas EINA_UNUSED, Evas_Object *object, void *event)
{
   Evas_Event_Mouse_Down *mouse = event;
   if (mouse->button != 1 || elm_win_maximized_get(data)) return;
   elm_win_move_resize_start(
     data, (Elm_Win_Move_Resize_Mode)(intptr_t)evas_object_data_get(object, "equation_resize"));
}

static void
equation_window_state_changed(void *data, Evas_Object *object EINA_UNUSED, void *event EINA_UNUSED)
{
   Equation_Window *chrome = data;
   elm_layout_signal_emit(
     chrome->frame,
     elm_win_maximized_get(chrome->window) ? "equation,maximized" : "equation,restored", "equation");
}

static void
equation_window_bind(Equation_Window *chrome)
{
   Evas_Object *edje = elm_layout_edje_get(chrome->frame);
   Evas_Object *move = (Evas_Object *)edje_object_part_object_get(edje, "equation.move");
   if (move)
     {
        evas_object_event_callback_del_full(move, EVAS_CALLBACK_MOUSE_DOWN, equation_window_move,
                                            chrome->window);
        evas_object_event_callback_add(move, EVAS_CALLBACK_MOUSE_DOWN, equation_window_move,
                                       chrome->window);
     }
   static const char *const names[] = { "top", "bottom", "left", "right", "tl", "tr", "bl", "br" };
   static const int modes[] = { ELM_WIN_MOVE_RESIZE_TOP,
                                 ELM_WIN_MOVE_RESIZE_BOTTOM,
                                 ELM_WIN_MOVE_RESIZE_LEFT,
                                 ELM_WIN_MOVE_RESIZE_RIGHT,
                                 ELM_WIN_MOVE_RESIZE_TOP | ELM_WIN_MOVE_RESIZE_LEFT,
                                 ELM_WIN_MOVE_RESIZE_TOP | ELM_WIN_MOVE_RESIZE_RIGHT,
                                 ELM_WIN_MOVE_RESIZE_BOTTOM | ELM_WIN_MOVE_RESIZE_LEFT,
                                 ELM_WIN_MOVE_RESIZE_BOTTOM | ELM_WIN_MOVE_RESIZE_RIGHT };
   for (unsigned i = 0; i < EINA_C_ARRAY_LENGTH(names); ++i)
     {
        Evas_Object *edge = (Evas_Object *)edje_object_part_object_get(edje, names[i]);
        if (!edge) continue;
        evas_object_data_set(edge, "equation_resize", (void *)(intptr_t)modes[i]);
        evas_object_event_callback_del_full(edge, EVAS_CALLBACK_MOUSE_DOWN, equation_window_resize,
                                            chrome->window);
        evas_object_event_callback_add(edge, EVAS_CALLBACK_MOUSE_DOWN, equation_window_resize,
                                       chrome->window);
     }
   elm_object_part_text_set(chrome->frame, "equation.title", chrome->title);
   if (chrome->dialog) elm_layout_signal_emit(chrome->frame, "equation,dialog", "equation");
   equation_window_state_changed(chrome, NULL, NULL);
}

static Evas_Object *
equation_window_control(Equation_Window *chrome, const char *text, const char *hint,
                        Evas_Smart_Cb callback)
{
   Evas_Object *button = elm_button_add(chrome->controls);
   if (!chrome->dialog) elm_object_focus_allow_set(button, EINA_FALSE);
   elm_object_style_set(button, callback == equation_window_close ? "equation/window-close"
                               : callback == equation_window_minimize ? "equation/window-minimize"
                                                                      : "equation/window");
   elm_object_text_set(button, text);
   evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(button, ELM_SCALE_SIZE(28), ELM_SCALE_SIZE(28));
   elm_object_tooltip_text_set(button, hint);
   elm_object_access_info_set(button, hint);
   if (callback) evas_object_smart_callback_add(button, "clicked", callback, chrome->window);
   elm_box_pack_end(chrome->controls, button);
   evas_object_show(button);
   return button;
}

static void
equation_window_free(void *data, Evas *canvas EINA_UNUSED, Evas_Object *object EINA_UNUSED,
                     void *event EINA_UNUSED)
{
   Equation_Window *chrome = data;
   eina_stringshare_del(chrome->title);
   free(chrome);
}

Evas_Object *
equation_window_add(Evas_Object *parent, const char *title, const char *path, Elm_Theme *theme)
{
   Equation_Window *chrome = calloc(1, sizeof(*chrome));
   if (!chrome) return NULL;
   chrome->title = eina_stringshare_add(title);
   chrome->dialog = parent != NULL;
   chrome->window = elm_win_add(parent, parent ? "equation_about" : "equation",
                                parent ? ELM_WIN_DIALOG_BASIC : ELM_WIN_BASIC);
   elm_win_title_set(chrome->window, title);
   elm_win_icon_name_set(chrome->window, "equation_calculator");
   Evas_Object *icon = elm_icon_add(chrome->window);
   if (elm_icon_standard_set(icon, "equation_calculator")
       || elm_image_file_set(icon, EQUATION_ICON_INSTALL, NULL)
       || elm_image_file_set(icon, EQUATION_ICON_SOURCE, NULL))
     elm_win_icon_object_set(chrome->window, icon);
   else
     evas_object_del(icon);
   elm_win_borderless_set(chrome->window, EINA_TRUE);
   elm_win_alpha_set(chrome->window, EINA_TRUE);
   elm_win_autodel_set(chrome->window, EINA_TRUE);
   elm_object_theme_set(chrome->window, theme);
   chrome->frame = elm_layout_add(chrome->window);
   if (!elm_layout_file_set(chrome->frame, path, "equation/frame"))
     {
        evas_object_del(chrome->window);
        eina_stringshare_del(chrome->title);
        free(chrome);
        return NULL;
     }
   evas_object_size_hint_weight_set(chrome->frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(chrome->window, chrome->frame);
   chrome->controls = elm_box_add(chrome->window);
   elm_box_horizontal_set(chrome->controls, EINA_TRUE);
   elm_box_padding_set(chrome->controls, 3, 0);
   if (!parent)
     {
        chrome->menu = equation_window_control(chrome, "☰", _("Application menu"), NULL);
        evas_object_name_set(chrome->menu, "equation_menu");
        elm_box_unpack(chrome->controls, chrome->menu);
        elm_object_part_content_set(chrome->frame, "equation.menu", chrome->menu);
        equation_window_control(chrome, "−", _("Minimize"), equation_window_minimize);
        equation_window_control(chrome, "□", _("Maximize or restore"), equation_window_maximize);
     }
   equation_window_control(chrome, "×", _("Close"), equation_window_close);
   elm_object_part_content_set(chrome->frame, "equation.controls", chrome->controls);
   evas_object_show(chrome->controls);
   equation_window_bind(chrome);
   evas_object_show(chrome->frame);
   evas_object_data_set(chrome->window, "equation_chrome", chrome);
   evas_object_smart_callback_add(chrome->window, "maximized", equation_window_state_changed,
                                  chrome);
   evas_object_smart_callback_add(chrome->window, "unmaximized", equation_window_state_changed,
                                  chrome);
   evas_object_event_callback_add(chrome->window, EVAS_CALLBACK_DEL, equation_window_free, chrome);
   return chrome->window;
}

void
equation_window_theme_set(Evas_Object *window, const char *path)
{
   Equation_Window *chrome = evas_object_data_get(window, "equation_chrome");
   if (!chrome) return;
   elm_object_part_content_unset(chrome->frame, "equation.content");
   elm_object_part_content_unset(chrome->frame, "equation.controls");
   if (chrome->menu) elm_object_part_content_unset(chrome->frame, "equation.menu");
   elm_layout_file_set(chrome->frame, path, "equation/frame");
   if (chrome->content)
     elm_object_part_content_set(chrome->frame, "equation.content", chrome->content);
   elm_object_part_content_set(chrome->frame, "equation.controls", chrome->controls);
   if (chrome->menu) elm_object_part_content_set(chrome->frame, "equation.menu", chrome->menu);
   equation_window_bind(chrome);
}

void
equation_window_content_set(Evas_Object *window, Evas_Object *content)
{
   Equation_Window *chrome = evas_object_data_get(window, "equation_chrome");
   chrome->content = content;
   elm_object_part_content_set(chrome->frame, "equation.content", content);
}

void
equation_window_menu_set(Evas_Object *window, Evas_Smart_Cb callback, void *data)
{
   Equation_Window *chrome = evas_object_data_get(window, "equation_chrome");
   if (chrome->menu) evas_object_smart_callback_add(chrome->menu, "clicked", callback, data);
}

void
equation_window_size_minimum(Evas_Object *window)
{
   Equation_Window *chrome = evas_object_data_get(window, "equation_chrome");
   /* Resolve widget and swallow hints before allocating the visible window. */
   Evas *canvas = evas_object_evas_get(window);
   evas_smart_objects_calculate(canvas);
   elm_layout_sizing_eval(chrome->frame);
   evas_smart_objects_calculate(canvas);
   int width, height;
   evas_object_size_hint_min_get(chrome->frame, &width, &height);
   evas_object_resize(window, width > 0 ? width : 1, height > 0 ? height : 1);
}
