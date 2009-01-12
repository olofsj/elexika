#include <stdlib.h>
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include <Elementary.h>
#include "elexika_dictionary.h"
#include "elexika_result_list.h"

static Eina_List * _get_available_dicts(void);
static const char * _user_homedir_get(void);

static Eina_List *_dicts = NULL;

static int
_cb_load_timer(void *data)
{
    Eina_List *result, *l, *dicts;
    Evas *evas;
    Evas_Object *en;
    char *file;

    evas = data;
    en = evas_object_name_find(evas, "results");
    elexika_result_list_message_show(en, "Loading dictionary");

    /* set up the dictionary */
    ecore_main_loop_iterate();
    dicts = _get_available_dicts();
    ecore_main_loop_iterate();


    for (l = dicts; l; l = l->next) {
        file = l->data;
        printf("Loading dictionary: %s\n", file);
        _dicts = eina_list_append(_dicts, 
                elexika_dictionary_new_from_file(file));
    }

    elexika_result_list_message_clear(en);

    return 0;
}

static void
_cb_query(void *data, Evas_Object *obj, void *event_info)
{
    Eina_List *result, *l, *d;
    Dictionary *dict;
    Evas *evas;
    Evas_Object *en, *entry;
    char *query;
    char *end;

    evas = data;
    en = evas_object_name_find(evas, "results");
    entry = evas_object_name_find(evas, "entry/query");
    elexika_result_list_message_show(en, "Querying...");

    query = strdup(elm_entry_entry_get(entry));

    /* Strip any markup at the end (for some reason it shows up) */
    end = strstr(query, "<");
    if (end)
        *end = '\0';

    /* Strip any ending whitespace */
    end = query + strlen(query) - 1;
    while (isspace(*end))
        *end-- = '\0';

    if (strlen(query) == 0)
        return;

    elexika_result_list_clear(en);

    printf("Query: %s\n", query);
    /* Query all loaded dictionaries and merge results */
    result = NULL;
    for (d = _dicts; d; d = d->next) {
        dict = d->data;
        result = eina_list_sorted_merge(result, elexika_dictionary_query(dict, query), 
                elexika_dictionary_sort_cb);
        printf("Queried dictionary '%s'.\n", dict->name);
    }

    /* Display results */
    if (result) {
        elexika_result_list_append(en, result);
    }

    elexika_result_list_message_clear(en);
    printf("Finished.\n");
}

static Eina_List *
_get_available_dicts(void)
{
    Ecore_List *files;
    char buf[PATH_MAX], *p, *file;
    const char *homedir, *fl;
    Eina_List *dicts = NULL, *l, *layouts = NULL;
    int ok;

    homedir = _user_homedir_get();
    snprintf(buf, sizeof(buf), "%s/.elexika", homedir);
    files = ecore_file_ls(buf);
    if (files)
    {
        ecore_list_first_goto(files);
        while ((file = ecore_list_current(files)))
        {
            p = strrchr(file, '.');
            if ((p) && (!strcmp(p, ".dict")))
            {
                snprintf(buf, sizeof(buf), "%s/.elexika/%s", homedir, file);
                dicts = eina_list_append(dicts, evas_stringshare_add(buf));
            }
            ecore_list_next(files);
        }
        ecore_list_destroy(files);
    }

    for (l = dicts; l; l = l->next) {
        printf("Availible dict: %s\n", l->data);
    }
    
    return dicts;
}

static const char *
_user_homedir_get(void)
{
    char *homedir;
    int len;

    homedir = getenv("HOME");
    if (!homedir) return "/tmp";
    len = strlen(homedir);
    while ((len > 1) && (homedir[len - 1] == '/'))
    {
        homedir[len - 1] = 0;
        len--;
    }
    return homedir;
}

static void
on_win_del_req(void *data, Evas_Object *obj, void *event_info)
{
    elm_exit();
}

EAPI int
elm_main(int argc, char **argv)
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
    elm_win_title_set(win, "Elexika");
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
    evas_object_smart_callback_add(entry, "activated", _cb_query, evas);
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

    o = elexika_result_list_add(evas, win);
    evas_object_name_set(o, "results");
    evas_object_size_hint_weight_set(o, 1.0, 1.0);
    evas_object_size_hint_align_set(o, -1.0, -1.0);
    elm_scroller_content_set(sc, o);
    evas_object_show(o);

    evas_object_show(sc);

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
ELM_MAIN()
