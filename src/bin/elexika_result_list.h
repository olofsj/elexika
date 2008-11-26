#ifndef _OMDICT_RESULT_LIST_H_
#define _OMDICT_RESULT_LIST_H_
   
#include <Evas.h>

Evas_Object *elexika_result_list_add(Evas *evas, Evas_Object *parent);
void elexika_result_list_clear(Evas_Object *obj);
void elexika_result_list_append(Evas_Object *obj, Eina_List *list);

#endif

