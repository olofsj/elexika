#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <Ecore.h>
#include "elexika_dictionary.h"

static int _dictionary_load_from_file(Dictionary *self, const char *filename, char **seps);
static int _build_dictionary_index(Dictionary *self);
static void _parse_format(const char *format, char ***seps, char ***fields);
static char * _format_result(char **match, int *length, char **fields, char *markup, const char *query);
static char * _get_config_value(char *buf);
static char * _strnstr(const char *big, const char *little, int len);

void
elexika_dictionary_del(Dictionary* self)
{
	/* Free all values stored in the arrays, as well as the arrays themselves */
	if (self->fields != NULL) {
		int size = 0;
		while ( self->fields[size] != NULL ) { size++; }
		int i;
		for (i = 0; i < size; i++) { free(self->fields[i]); }
	}
	free(self->fields); 
	if (self->seps != NULL) {
		int size = 0;
		while ( self->seps[size] != NULL ) { size++; }
		int i;
		for (i = 0; i < size; i++) { free(self->seps[i]); }
	}
	free(self->seps); 
	free(self->format);
    // FIXME: Need to free all the self->file objects as well
}

Dictionary *
elexika_dictionary_new_from_file(const char *filename)
{
   FILE *f;
   char buf[4096];
   int isok = 0;
   char *name, *file, *markup, *format;
   
   f = fopen(filename, "r");
   if (!f) return NULL;

   while (fgets(buf, sizeof(buf), f))
   {
       int len;
       char str[4096];

       if (!isok)
       {
           if (!strcmp(buf, "##DICTCONF-1.0\n")) isok = 1;
       }
       if (!isok) break;
       if (buf[0] == '#') continue;
       len = strlen(buf);
       if (len > 0)
       {
           if (buf[len - 1] == '\n') buf[len - 1] = 0;
       }
       if (sscanf(buf, "%4000s", str) != 1) continue;
       if (!strcmp(str, "name"))
       {
           name = _get_config_value(buf+strlen(str));
           printf("Matched name: %s\n", name);
       }
       if (!strcmp(str, "file"))
       {
           file = _get_config_value(buf+strlen(str));
           printf("Matched file: %s\n", file);
       }
       if (!strcmp(str, "markup"))
       {
           markup = _get_config_value(buf+strlen(str));
           printf("Matched markup: %s\n", markup);
       }
       if (!strcmp(str, "format"))
       {
           format = _get_config_value(buf+strlen(str));
           printf("Matched format: %s\n", format);
       }
   }
   fclose(f);

   return elexika_dictionary_new(name, file, format, markup);
}

Dictionary *
elexika_dictionary_new(const char *name, const char *filename, const char *format, const char *markup)
{
    Dictionary *dict;

    dict = calloc(1, sizeof(Dictionary));
    dict->name = strdup(name);
    dict->dict.size = 0;
    dict->dict.field = NULL;
    dict->dict.length = NULL;
    dict->format =  strdup(format);
    dict->markup =  strdup(markup);
    dict->seps =  NULL;
    dict->fields =  NULL;
    dict->max_nrof_results = 50;
    dict->file.file = strdup(filename);
    dict->file.fd = -1;
    dict->file.size = 0;

    _parse_format(format, &dict->seps, &dict->fields);	
    if (!_dictionary_load_from_file(dict, filename, dict->seps))
        return NULL;
	
	return dict;
}

/* Look up a word in the dictionary */
Eina_List *
elexika_dictionary_query(Dictionary *self, const char *str)
{
	const char *query;
	Eina_List *result;
    Match *match;

    query = str;
    result = NULL;
	
	if ( self->dict.field == NULL ) {
		printf("No dictionary loaded.\n");
		return NULL;
	}
	if ( strlen(query) == 0 )
		return NULL;
	
	/* Search the dictionary. FIXME: We might want to implement a reasonable search algorithm. */
	if (self->dict.size == 0) {
		printf("No dictionary loaded.\n");
	}
	else {
		int i, j, k;
		int count = 0;
		for (i = 0; i < self->dict.size; i++) {
			int isize = 0;
			while ( self->dict.field[i][isize] != NULL ) { isize++; }
			
			/* Look in all fields for matches */
			for (k = 0; k<isize; k++) {
				if (_strnstr(self->dict.field[i][k], query, self->dict.length[i][k]) != NULL ) {
					/* Format and calculate the score */
					int score = self->dict.length[i][k];
					score += k;		// Penalty for being in later fields, usually gives a nice result

                    /* If there are enough better matches, skip this one */
                    Match *last = eina_list_data_get(eina_list_last(result));
                    if (result && score > last->score && count >= self->max_nrof_results) 
                        break;

                    /* Build return value */
                    match = calloc(1, sizeof(Match));
                    match->score = score;
                    match->str = _format_result(self->dict.field[i], self->dict.length[i], 
                            self->fields, self->markup, query);
                    Eina_List *l = eina_list_append(NULL, match);
					
					/* Insert any matches into the return value list */
                    result = eina_list_sorted_merge(result, l, elexika_dictionary_sort_cb);

                    if (++count > self->max_nrof_results)
                        result = eina_list_remove_list(result, eina_list_last(result));
					
					/* Don't add same word multiple times */
					break; 
				}
			}
		}
	}

    return result;
}

int
elexika_dictionary_sort_cb(const void *d1, const void *d2)
{
    const Match *m1 = NULL;
    const Match *m2 = NULL;

    if (!d1) return(1);
    if (!d2) return(-1);

    m1 = d1;
    m2 = d2;

    if (m1->score > m2->score)
        return 1;
    
    return -1;
}

/* Get the number of entries in the dictionary */
int
elexika_dictionary_size_get(Dictionary *self)
{
	return self->dict.size;
}


/* Load the dictionary into memory */
static int _dictionary_load_from_file(Dictionary *self, const char *filename, char **seps)
{
    double start, end;
    start = ecore_time_get();

    struct stat st;

    /* Open file descriptor and stat the file to get size */
    self->file.fd = open(self->file.file, O_RDONLY);
    if (self->file.fd < 0)
        return 0;
    if (fstat(self->file.fd, &st) < 0)
    {
        close(self->file.fd);
        return 0;
    }
    self->file.size = st.st_size;

    /* mmap file contents */
    self->file.dict = mmap(NULL, self->file.size, PROT_READ, MAP_SHARED, self->file.fd, 0);
    if ((self->file.dict== MAP_FAILED) || (self->file.dict == NULL))
    {
        close(self->file.fd);
        return 0;
    }

    _build_dictionary_index(self);
	
	/* Clean up */
    end = ecore_time_get();
	printf( "Dictionary loaded in %f seconds.\n", end - start);
    return 1;
}

static int _build_dictionary_index(Dictionary *self) {
	/* Parse the dictionary and store it in the global variable dictionary*/	
	/* Get number of lines in the file */
	int nr_lines = 0;
	char * current = self->file.dict;
	while ( current[0] != '\0' ) {
		if ( current [0] == '\n' ) { nr_lines++; }
		current++;
	}
	printf( "Dictionary size: %d\n", nr_lines );
	
	/* Allocate memory */
	char ***dict = malloc( (nr_lines+1) * sizeof(char **) );
	int **length = malloc( (nr_lines+1) * sizeof(int *) );
	if ( dict == NULL ) {
		printf( "Insufficient memory to parse dictionary.\n" );
		return 0;
	}

    /**/
	int nr_of_seps = 0;
	if (self->seps != NULL) {
		while ( self->seps[nr_of_seps] != NULL ) { nr_of_seps++; }
	}

    /* Build the index of starts and lengths of fields */
    char *line, *line_end;
    int k;
    int i = 0;
    line = self->file.dict;
    printf("Build the index.\n");
    while (line < self->file.dict + self->file.size) {
        line_end = line;
        while (*line_end && *line_end != '\n')
            line_end++;

        /* If we have no reasonable format data just return the whole line */
        if (nr_of_seps < 2) { 
            dict[i] = malloc(2 * sizeof(char *));
            dict[i][0] = line;
            dict[i][1] = NULL;
            length[i] = malloc(sizeof(int ));
            length[i][0] = line_end - line;
        }
        else {
            /* Else split on separators */ 
            dict[i] = malloc((nr_of_seps) * sizeof(char *));
            length[i] = malloc((nr_of_seps-1) * sizeof(int));

            char *pos = strstr( line, self->seps[0] );
            if ( pos == NULL ) {
                // Illegal format, return without parsing
                free(dict[i]);
                free(length[i]);
                line = line_end+1;
                continue;
            }
            pos = pos + strlen(self->seps[0]);

            char *end;
            char *begin;
            /* Parse all but the last field */ 
            for (k = 0; k < nr_of_seps - 2; k++) {
                begin = pos;
                end = strstr( pos, self->seps[k+1] );
                if ( begin == NULL || end == NULL) {
                    // Illegal format, return without parsing
                    free(dict[i]);
                    free(length[i]);
                    line = line_end+1;
                    continue;
                }
                dict[i][k] = begin;
                length[i][k] = end - begin;
                if (end-begin < 0) {
                    printf("ERROR: Negative length\n");
                }
                pos = end + strlen(self->seps[k+1]);
            }

            /* The last field we take the last separator from the end, not the first match */
            begin = pos;
            //end = (char *)&line[ line_end - line - strlen( self->seps[nr_of_seps-1] ) ];
            end = line_end - strlen( self->seps[nr_of_seps-1] );
            if ( strncmp( end, self->seps[nr_of_seps-1], line_end - end ) == 0 && end-begin >= 0) {
                dict[i][k] = begin;
                length[i][k] = end - begin;
            } else {
                // Illegal format, return without parsing
                free(dict[i]);
                free(length[i]);
                line = line_end+1;
                continue;
            }
            dict[i][nr_of_seps-1] = NULL;
        }

        line = line_end+1;
        i++;
    }
    dict[i] = NULL;
    self->dict.size = i-1;
    self->dict.field = dict;
    self->dict.length = length;
    printf("Parsed %d entries.\n", i);
    printf("First entry : %s (%d)\n", strndup(self->dict.field[0][0], self->dict.length[0][0]), self->dict.length[0][0]);
    printf("Last entry : %s (%d)\n", strndup(self->dict.field[i-1][0], self->dict.length[i-1][0]), self->dict.length[i-1][0]);

}

/* Parse the format data and store extract the separators and field names */
static void _parse_format(const char *format, char ***seps, char ***fields) {
	/* Parse the format data */
	int i;
	int nr_of_fields = 0;
	for (i = 0; i < strlen(format); i++)
		if (format[i] == '%') 
			nr_of_fields++;
	
	char ** tempseps = calloc((nr_of_fields+2), sizeof(char *));
	char ** tempfields = calloc((nr_of_fields+1), sizeof(char *));
	
	int last = 0; 
	int sep = 0;
	for (i = 0; i < strlen(format); i++) {
		if (format[i] == '%') {
			tempseps[sep] = calloc((i - last +1), sizeof(char));
			memcpy( tempseps[sep], &format[last], i - last);
			tempseps[sep][i - last] = '\0';
			last = i+2;
			tempfields[sep] = calloc(3, sizeof(char));
			tempfields[sep][0] = '%';
			tempfields[sep][1] = format[i+1];
			tempfields[sep][2] = '\0';
			i++; sep++;
		}
	} 
	tempseps[sep] = strdup(&format[last]);
	tempseps[sep+1] = NULL;
	tempfields[sep] = NULL;
	
	// FIXME: free seps and fields if not NULL (though only used initially so no biggie)
	*seps = tempseps;
	*fields = tempfields;
}

/* Format result according to given markup string */
static char * _format_result(char **match, int *length, char **fields, char *markup, const char *query) {
	int k, j;
    const char * match_tag = "match";

	/* If we have no format or markup just return the first field in match */
	if ( fields == NULL || markup == NULL ) 
		return strndup(match[0], length[0]);
	if ( strlen(markup) == 0 ) 
		return strndup(match[0], length[0]);
		
	int nr_of_fields = 0;
	while ( fields[nr_of_fields] != NULL ) { nr_of_fields++; }
	
	/* If the match does not contain the given number of fields, just return it as it is */
	int size = 0;
	while ( match[size] != NULL ) { size++; }
	if (size != nr_of_fields)
		return strndup(match[0], length[0]);

    /* Count the number of matching substrings */
    int nrof_matches = 0;
	for (k = 0; k < nr_of_fields; k++) {
        int len = length[k];
        char *begin = match[k];
        char *end = _strnstr(begin, query, len);
        while (end) {
            nrof_matches++;
            len -= end + strlen(query) - begin;
            begin = end + strlen(query);
            end = _strnstr(begin, query, len);
        }
    }
	
	/* Set the length of the returned string as the length of 
     * markup + fields + nrof_matches * tags */
	int len = strlen(markup) + nrof_matches*(5 + 2*strlen(match_tag)) - 2*nr_of_fields;
	for (k = 0; k < nr_of_fields; k++) 
		len = len + length[k];

	char *result = calloc(len+1, sizeof(char));
	
	/* Build the string from format and fields */
	int pos = 0;
	k = 0;
	while ( k < strlen(markup) ) {
		if ( markup[k] == '%' ) {
			int field = -1;
			for (j=0; j < nr_of_fields; j++) {
				if ( fields[j][1] == markup[k+1] )
					field = j;
			}
			if (field != -1) {
                int len = length[field];
                char *begin = match[field];
                char *end = _strnstr(begin, query, len);
                while (end) {
                    /* Add tags around each matching substring */
                    strncpy(&result[pos], begin, end - begin);
                    pos += end - begin;
                    result[pos++] = '<';
                    strcpy(&result[pos], match_tag);
                    pos += strlen(match_tag);
                    result[pos++] = '>';
                    strcpy(&result[pos], query);
                    pos += strlen(query);
                    result[pos++] = '<';
                    result[pos++] = '/';
                    strcpy(&result[pos], match_tag);
                    pos += strlen(match_tag);
                    result[pos++] = '>';
                    len -= end + strlen(query) - begin;
                    begin = end + strlen(query);
                    end = _strnstr(begin, query, len);
                }
				strncpy( &result[pos], begin, len );
				pos += len;
				k = k+2;
			} else {
				result[pos]=markup[k];
				pos++;
				k++;
			}
		} else {
			result[pos]=markup[k];
			pos++;
			k++;
		}
	}
	result[pos] = '\0';

    /*
    printf("nrof_matches = %d\n", nrof_matches);
    printf("%d\n", pos - len);
    printf("Entry: ");
	for (k = 0; k < nr_of_fields; k++) {
        printf("(%d) ", length[k]);
        printf("%s ", strndup(match[k], length[k]));
    }
    printf("\n");
    printf("Formatted string: %s\n", result);
    */
	return result;
}

/* Read the config option from the buffer, i.e. strip unneeded chars. */
static char * _get_config_value(char *buf)
{
    char *start, *end, quote;
    start = buf;
    while (*start == ' ' || *start == '\t' || *start == '=')
        start++;
    if (*start == '"' || *start == '\'') {
        quote = *start;
        start++;
    }
    end = start;
    while (*end && *end != quote && *end != '\n')
        end++;
    *end = '\0';
    return strdup(start);
}

static char * _strnstr(const char *big, const char *little, int len)
{
    char *pos = big;
    //printf("len = %d\n", len);
    //printf("Look for %s in %s (%d)\n", little, strndup(big, len), len);
    while (pos < big + len - strlen(little) + 1) {
        if (!*pos)
            return NULL;

        if (*pos == *little) {
            int i = 1;
            while (pos[i] == little[i] && i < strlen(little)) {
                i++;
            }
            if (i == strlen(little)) {
                return pos;
            }
        }
        pos++;
    }
    return NULL;
}
