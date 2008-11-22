#ifndef _OMDICT_DICTIONARY_H_
#define _OMDICT_DICTIONARY_H_ 
   
#include <Eina.h>

typedef struct _Dictionary Dictionary;
typedef struct _Match Match;

struct _Dictionary {
	char *name; 
	char *filename; 
	char *format; 
	char *markup; 
	char **seps; 
	char **fields; 
	char ***dict;
};

struct _Match {
    int score;
    char *str;
};

Dictionary *omdict_dictionary_new_from_file(const char *filename);
Dictionary *omdict_dictionary_new(const char *name, const char *filename, const char *format, const char *markup);
void omdict_dictionary_del(Dictionary* self);
Eina_List *omdict_dictionary_query(Dictionary *self, const char *str);
int omdict_dictionary_size_get(Dictionary *self);
int omdict_dictionary_sort_cb(const void *d1, const void *d2);

#endif
