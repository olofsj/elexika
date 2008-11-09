#include <stdlib.h>
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>
#include "omdict_dictionary.h"

int
main(int argc, char **argv)
{
    Ecore_Evas *ee;
    Evas *evas;
    Evas_Object *bg, *edje, *o, *canvas;
    Evas_Coord w, h;
    Dictionary *dict;
    Eina_List *result;
    Match *match;
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

    /* show the window */
    ecore_evas_show(ee);

    /* set up the dictionary */
    dict = omdict_dictionary_new(
            "Edict", 
            "/home/olof/code/JapaScanMulti/dicts/edict.mod.txt", 
            "\"/%1/\"	\"/%2/\"	\"/%3/\"	\"%4\"", 
            "%2<br>%1 %4<br>%3");

    printf("Entries: %d\n", omdict_dictionary_size_get(dict));
    result = omdict_dictionary_query(dict, "school");
    match = result->data;
    printf("Result: %s\n", match->str);
    edje_object_part_text_set(edje, "results", match->str);

    /* start the main event loop */
    ecore_main_loop_begin();

    /* when the main event loop exits, shutdown our libraries */
    ecore_evas_shutdown();
    ecore_shutdown();
    evas_shutdown();
}
