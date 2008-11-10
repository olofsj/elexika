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

int
main(int argc, char **argv)
{
    Ecore_Evas *ee;
    Evas *evas;
    Evas_Object *bg, *edje, *o, *canvas;
    Evas_Coord w, h;
    w = 480;
    h = 640;

    /* initialize our libraries */
    evas_init();
    ecore_init();
    ecore_evas_init();
    edje_init();

    /* create our Ecore_Evas */
    ee = ecore_evas_software_x11_new(0, 0, 0, 0, w, h);
    ecore_evas_title_set(ee, "OmDict");

    /* get a pointer our new Evas canvas */
    evas = ecore_evas_get(ee);

    /* Load and set up the edje objects */
    edje = edje_object_add(evas);
    edje_object_file_set(edje, "../../data/themes/omdict.edj", "omdict");
    evas_object_move(edje, 0, 0);
    evas_object_resize(edje, w, h);
    evas_object_show(edje);
    ecore_timer_add(2.0, _cb_load_timer, edje);
    //edje_object_signal_callback_add(edje, "load", "", _cb_show, NULL);

    /* show the window */
    ecore_evas_show(ee);

    /* start the main event loop */
    ecore_main_loop_begin();

    /* when the main event loop exits, shutdown our libraries */
    ecore_evas_shutdown();
    ecore_shutdown();
    evas_shutdown();
}
