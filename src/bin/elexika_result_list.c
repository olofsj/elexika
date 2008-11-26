#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Job.h>
#include <Edje.h>
#include "elexika_result_list.h" 
#include "elexika_dictionary.h" 

typedef struct _Smart_Data Smart_Data;

struct _Smart_Data
{ 
    Evas_Coord       x, y, w, h;
    Evas_Object     *parent;
    Evas_Object     *obj;
    Evas_Object     *clip;
    Evas_Object     *bg;
    Eina_List       *children;
    Ecore_Job       *deferred_recalc_job;
}; 

/* local subsystem functions */
static void _smart_init(void);
static void _smart_add(Evas_Object *obj);
static void _smart_del(Evas_Object *obj);
static void _smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _smart_show(Evas_Object *obj);
static void _smart_hide(Evas_Object *obj);
static void _smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _smart_clip_unset(Evas_Object *obj);
static void _smart_parent_set(Evas_Object *obj, Evas_Object *parent);
static void _sizing_eval(Evas_Object *obj);
static void _recalc_job(void *data);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* externally accessible functions */
Evas_Object *
elexika_result_list_add(Evas *evas, Evas_Object *parent)
{
    Evas_Object *obj;

    _smart_init();
    obj = evas_object_smart_add(evas, _e_smart);
    _smart_parent_set(obj, parent);
    return obj;
}

void
elexika_result_list_clear(Evas_Object *obj)
{
    Smart_Data *sd;
    Eina_List *l;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;

    for (l = sd->children; l; l = l->next) {
        sd->children = eina_list_remove(sd->children, l->data);
    }

    _sizing_eval(obj);
}

void
elexika_result_list_append(Evas_Object *obj, Eina_List *list)
{
    Evas_Object *o;
    Smart_Data *sd;
    Evas_Coord minw, minh;
    Eina_List *l;
    Match *match;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;

    for (l = list; l; l = l->next) {
        match = l->data;
        o = edje_object_add(evas_object_evas_get(sd->obj));
        edje_object_file_set(o, "../../data/themes/elexika.edj", "result");
        edje_object_part_text_set(o, "result.text", match->str);
        //printf("Appending text to list: %s\n", str);

        evas_object_show(o);

        evas_object_clip_set(o, sd->clip);
        evas_object_smart_member_add(o, obj);
        sd->children = eina_list_append(sd->children, o);
        if (eina_list_count(sd->children) % 2 == 0)
            edje_object_signal_emit(o, "result,state,even", "result_list");
    }

    _sizing_eval(obj);
}

/* callbacks */
static void
_recalc_job(void *data)
{
    Evas_Coord minw = -1, minh = -1, maxw = -1, maxh = -1;
    Evas_Coord resw, resh, minminw;
    Eina_List *l;
    Evas_Object *child;
    Smart_Data *sd;

    sd = evas_object_smart_data_get(data);
    if (!sd) return;

    sd->deferred_recalc_job = NULL;

    resh = 0;
    for (l = sd->children; l; l = l->next) {
        child = l->data;
        edje_object_size_min_restricted_calc(child, &minw, &minh, sd->w, 0);
        evas_object_resize(child, sd->w, minh);
        evas_object_move(child, sd->x, sd->y + resh);
        resh = resh + minh;
        //printf("Child Size: %d %d\n", sd->w, minh);
    }

    evas_object_resize(sd->bg, sd->w, resh);
    evas_object_resize(sd->clip, sd->w, resh);
    evas_object_size_hint_min_set(sd->obj, 0, resh);
    evas_object_size_hint_max_set(sd->obj, maxw, resh);
}

/* private functions */
static void
_sizing_eval(Evas_Object *obj)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;

    if (sd->deferred_recalc_job) ecore_job_del(sd->deferred_recalc_job);
    sd->deferred_recalc_job = ecore_job_add(_recalc_job, obj);
}

static void
_smart_init(void)
{
    if (_e_smart) return;
    {
        static const Evas_Smart_Class sc =
        {
            "elexika_result_list",
            EVAS_SMART_CLASS_VERSION,
            _smart_add,
            _smart_del,
            _smart_move,
            _smart_resize,
            _smart_show,
            _smart_hide,
            _smart_color_set,
            _smart_clip_set,
            _smart_clip_unset,
            NULL,
            NULL,
            NULL,
            NULL
        };
        _e_smart = evas_smart_class_new(&sc);
    }
}

static void
_smart_add(Evas_Object *obj)
{
    Smart_Data *sd;

    /* Initialize smart data and clipping */
    sd = calloc(1, sizeof(Smart_Data));
    if (!sd) return;
    sd->obj = obj;
    sd->parent = NULL;
    sd->children = NULL;
    sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
    evas_object_smart_member_add(sd->clip, obj);
   
    /* Set up edje object and canvas */
    sd->bg = edje_object_add(evas_object_evas_get(obj));
    edje_object_file_set(sd->bg, "../../data/themes/elexika.edj", "result_list");
    evas_object_move(sd->bg, 0, 0);
    evas_object_clip_set(sd->bg, sd->clip);
    evas_object_smart_member_add(sd->bg, obj);
    evas_object_show(sd->bg);

    evas_object_smart_data_set(obj, sd);
}

static void
_smart_del(Evas_Object *obj)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;
    evas_object_del(sd->clip);
    evas_object_del(sd->bg);
    elexika_result_list_clear(obj);
    if (sd->deferred_recalc_job) ecore_job_del(sd->deferred_recalc_job);
    free(sd);
}

static void
_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
    Smart_Data *sd;
    Eina_List *l;
    Evas_Coord dx, dy, curx, cury;
    Evas_Object *child;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;
    dx = x - sd->x;
    dy = y - sd->y;
    sd->x = x;
    sd->y = y;
    evas_object_move(sd->clip, x, y);
    evas_object_move(sd->bg, x, y);
    for (l = sd->children; l; l = l->next) {
        child = l->data;
        evas_object_geometry_get(child, &curx, &cury, NULL, NULL);
        evas_object_move(child, curx + dx, cury + dy);
    }
}

static void
_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
    Smart_Data *sd;
    Eina_List *l;
    Evas_Object *child;
    Evas_Coord minw, minh, resh;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;
    
    /*
    resh = 0;
    for (l = sd->children; l; l = l->next) {
        child = l->data;
        edje_object_size_min_restricted_calc(child, &minw, &minh, w, 0);
        evas_object_resize(child, w, minh);
        evas_object_move(child, sd->x, sd->y + resh);
        resh = resh + minh;
    }
    */

    sd->w = w;
    sd->h = h;
    //evas_object_resize(sd->clip, w, h);
    _sizing_eval(obj);

}

static void
_smart_show(Evas_Object *obj)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;
    evas_object_show(sd->clip);
}

static void
_smart_hide(Evas_Object *obj)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;
    evas_object_hide(sd->clip);
}

static void
_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;   
    evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;
    evas_object_clip_set(sd->clip, clip);
}

static void
_smart_clip_unset(Evas_Object *obj)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;
    evas_object_clip_unset(sd->clip);
}  

static void
_smart_parent_set(Evas_Object *obj, Evas_Object *parent)
{
    Smart_Data *sd;

    sd = evas_object_smart_data_get(obj);
    if (!sd) return;
    sd->parent = parent;
}  

