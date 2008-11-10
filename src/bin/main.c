#include <stdlib.h>
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include "omdict_dictionary.h"

static int
_cb_load_timer(void *data)
{
    Dictionary *dict;
    Eina_List *result;
    Match *match;
    Evas_Object *edje;

    edje = data;

    /* set up the dictionary */
    printf("Loading dictionary.\n");
    edje_object_part_text_set(edje, "status", "Loading dictionary.");
    dict = omdict_dictionary_new(
            "Edict", 
            "/home/olof/code/JapaScanMulti/dicts/edict.mod.txt", 
            "\"/%1/\"	\"/%2/\"	\"/%3/\"	\"%4\"", 
            "%2<br>%1 %4<br>%3");

    printf("Entries: %d\n", omdict_dictionary_size_get(dict));
    edje_object_part_text_set(edje, "status", "Querying dictionary.");
    result = omdict_dictionary_query(dict, "school");
    match = result->data;
    printf("Result: %s\n", match->str);
    edje_object_part_text_set(edje, "results", match->str);
    edje_object_part_text_set(edje, "status", "Ready.");
    return 0;
}

void 
_cb_resize(Ecore_Evas *ee) {
    int w, h;
    Evas_Object *o = NULL;

    if (ee) {
        ecore_evas_geometry_get(ee, NULL, NULL, &w, &h);
        if ((o = evas_object_name_find(ecore_evas_get(ee), "edje")))
            evas_object_resize(o, w, h);
    }
}

int
main(int argc, char **argv)
{
    Ecore_Evas *ee;
    Evas *evas;
    Evas_Object *bg, *edje, *o, *canvas;
    Evas_Coord x, y, w, h;
    w = 480;
    h = 640;

    /* initialize our libraries */
    evas_init();
    ecore_init();
    ecore_evas_init();
    edje_init();

    /* create our Ecore_Evas */
    ee = ecore_evas_new(NULL, 0, 0, w, h, ""); 
    ecore_evas_title_set(ee, "OmDict");
    ecore_evas_callback_resize_set(ee, _cb_resize);

    /* get a pointer our new Evas canvas */
    evas = ecore_evas_get(ee);

    /* Load and set up the edje objects */
    edje = edje_object_add(evas);
    edje_object_file_set(edje, "../../data/themes/omdict.edj", "omdict");
    evas_object_resize(edje, w, h);
    evas_object_show(edje);
    ecore_timer_add(1.0, _cb_load_timer, edje);
    evas_object_name_set(edje, "edje");

    /* show the window */
    ecore_evas_show(ee);

    /* start the main event loop */
    ecore_main_loop_begin();

    /* when the main event loop exits, shutdown our libraries */
    ecore_evas_shutdown();
    ecore_shutdown();
    evas_shutdown();
}
