#include <stdlib.h>
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include <Elementary.h>
#include "omdict_dictionary.h"

static Dictionary *_dict = NULL;

static int
_cb_load_timer(void *data)
{
    Eina_List *result, *l;
    Match *match;
    Evas *evas;
    Evas_Object *label, *en;

    evas = data;
    label = evas_object_name_find(evas, "label/status");
    en = evas_object_name_find(evas, "results");

    /* set up the dictionary */
    ecore_main_loop_iterate();
    printf("Loading dictionary.\n");
    elm_label_label_set(label,
            "<br>"
            "<b>Status: Loading dictionary...</b><br>"
            "<br>");
    ecore_main_loop_iterate();

    _dict = omdict_dictionary_new(
            "Edict", 
            "/home/olof/code/JapaScanMulti/dicts/edict.mod.txt", 
            "\"/%1/\"	\"/%2/\"	\"/%3/\"	\"%4\"", 
            "%2<br>%1 %4<br>%3<br><br>");

    elm_label_label_set(label,
            "<br>"
            "<b>Status: Ready.</b><br>"
            "<br>");
    return 0;
}

static void
_cb_query(void *data, Evas_Object *obj, void *event_info)
{
    Eina_List *result, *l;
    Match *match;
    Evas *evas;
    Evas_Object *label, *en, *entry;
    const char *query;
    char *end;

    evas = data;
    label = evas_object_name_find(evas, "label/status");
    en = evas_object_name_find(evas, "results");
    entry = evas_object_name_find(evas, "entry/query");

    query = elm_entry_entry_get(entry);

    /* Strip any markup at the end (for some reason it shows up) */
    end = strstr(query, "<");
    if (end) {
        *end = '\0';
    }
    if (strlen(query) == 0)
        return;

    elm_label_label_set(label,
            "<br>"
            "<b>Status: Querying dictionary...</b><br>"
            "<br>");
    elm_entry_entry_set(en, "Querying...");

    printf("Query: %s\n", query);
    result = omdict_dictionary_query(_dict, query);
    printf("Queried dictionary.\n");
    
    if (result) {
        printf("Got some results.\n");
        elm_entry_entry_set(en, "");
        for (l = result; l; l = l->next) {
            match = l->data;
            elm_entry_entry_insert(en, match->str);
        }
    } else {
        printf("No results.\n");
        elm_entry_entry_set(en, "No results.");
    }

    elm_label_label_set(label,
            "<br>"
            "<b>Status: Ready.</b><br>"
            "<br>");
    printf("Finished.\n");
}


static void
on_win_del_req(void *data, Evas_Object *obj, void *event_info)
{
   elm_exit();
}

int
main(int argc, char **argv)
{
    Ecore_Evas *ee;
    Evas *evas;
    Evas_Object *bg, *tb, *edje, *o, *sc, *en, *win, *bx, *bx2, *entry, *label, *button;
    Evas_Coord x, y, w, h;

    w = 240;
    h = 320;
    
    /* Initialize libraries */
    elm_init(argc, argv);

    /* Set up window */
    win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
    elm_win_title_set(win, "OmDict");
    evas_object_smart_callback_add(win, "delete-request", on_win_del_req, NULL);
    evas = evas_object_evas_get(win);

    bg = elm_bg_add(win);
    evas_object_size_hint_weight_set(bg, 1.0, 1.0); // expand h/v 1/1 (for win this also fills)
    elm_win_resize_object_add(win, bg);
    evas_object_show(bg);

    bx = elm_box_add(win);
    evas_object_size_hint_weight_set(bx, 1.0, 1.0);
    elm_win_resize_object_add(win, bx);
    evas_object_show(bx);

    bx2 = elm_box_add(win);
    elm_box_horizontal_set(bx2, 1);
    evas_object_size_hint_weight_set(bx2, 1.0, 0.0);
    evas_object_size_hint_align_set(bx2, -1.0, -1.0);
    elm_box_pack_end(bx, bx2);
    evas_object_show(bx2);
  
    label = elm_label_add(win);
    elm_label_label_set(label, "<b>Search:</b> ");
    evas_object_size_hint_weight_set(label, 0.0, 0.0);
    evas_object_size_hint_align_set(label, -1.0, 0.5);
    elm_box_pack_end(bx2, label);
    evas_object_show(label);

    sc = elm_scroller_add(win);
    elm_scroller_content_min_limit(sc, 0, 1);
    evas_object_size_hint_weight_set(sc, 1.0, 0.0);
    evas_object_size_hint_align_set(sc, -1.0, 0.5);
    elm_box_pack_end(bx2, sc);
    evas_object_show(sc);

    entry = elm_entry_add(win);
    elm_entry_single_line_set(entry, 1);
    elm_entry_entry_set(entry, "");
    elm_entry_editable_set(entry, 1);
    evas_object_size_hint_weight_set(entry, 1.0, 0.0);
    evas_object_size_hint_align_set(entry, -1.0, 0.5);
    evas_object_name_set(entry, "entry/query");
    elm_scroller_content_set(sc, entry);
    evas_object_show(entry);

    button = elm_button_add(win);
    elm_button_label_set(button, "Search");
    evas_object_smart_callback_add(button, "clicked", _cb_query, evas);
    evas_object_size_hint_weight_set(button, 0.0, 0.0);
    evas_object_size_hint_align_set(button, -1.0, 0.0);
    elm_box_pack_end(bx2, button);
    evas_object_show(button);

    sc = elm_scroller_add(win);
    evas_object_size_hint_weight_set(sc, 1.0, 1.0);
    evas_object_size_hint_align_set(sc, -1.0, -1.0);
    elm_box_pack_end(bx, sc);

    en = elm_entry_add(win);
    elm_entry_entry_set(en, "No results.");
    elm_entry_editable_set(en, 0);
    evas_object_size_hint_weight_set(en, 1.0, 1.0);
    evas_object_size_hint_align_set(en, -1.0, -1.0);
    elm_scroller_content_set(sc, en);
    evas_object_name_set(en, "results");
    evas_object_show(en);

    evas_object_show(sc);

    label = elm_label_add(win);
    elm_label_label_set(label,
            "<br>"
            "<b>Status: Initializing interface...</b><br>"
            "<br>");
    elm_box_pack_end(bx, label);
    evas_object_name_set(label, "label/status");
    evas_object_size_hint_weight_set(label, 1.0, 0.0);
    evas_object_size_hint_align_set(label, 0.0, 1.0);
    evas_object_show(label);

    /* start timer to defer loading of dictionary */
    ecore_timer_add(1.0, _cb_load_timer, evas);

    /* start the main event loop */
    evas_object_resize(win, w, h);
    evas_object_show(win);
    elm_widget_focus_set(entry, 1);
    elm_run();

    /* when the main event loop exits, shutdown our libraries */
    elm_shutdown();
}
