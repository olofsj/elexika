#ifndef _OMDICT_RESULT_LIST_H_
#define _OMDICT_RESULT_LIST_H_
   
#include <Evas.h>

Evas_Object *omdict_result_list_add(Evas *evas, Evas_Object *parent);
void omdict_result_list_clear(Evas_Object *obj);
void omdict_result_list_append(Evas_Object *obj, char * str);

#endif

