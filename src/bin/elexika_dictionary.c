#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elexika_dictionary.h"

static char *** _dictionary_load_from_file(const char *filename, char **seps);
static char ** _parse_input_line(const char *line, char **seps);
static void _parse_format(const char *format, char ***seps, char ***fields);
static char * _format_result(char **match, char **fields, char *markup, const char *query);
static char * _get_config_value(char *buf);

void
elexika_dictionary_del(Dictionary* self)
{
	/* Free all values stored in the arrays, as well as the arrays themselves */
	if (self->dict != NULL) {
		int size = 0;
		while ( self->dict[size] != NULL ) { size++; }
		int i;
		for (i = 0; i < size; i++) {
			int isize = 0;
			while ( self->dict[i][isize] != NULL ) { isize++; }
			int k;
			for (k = 0; k < isize; k++) { free(self->dict[i][k]); }
			free(self->dict[i]);
		}
	}
	free(self->dict);	
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
	free(self->filename);
	free(self->format);
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
    dict->filename = strdup(filename);
    dict->dict = NULL;
    dict->format =  strdup(format);
    dict->markup =  strdup(markup);
    dict->seps =  NULL;
    dict->fields =  NULL;
    dict->max_nrof_results = 50;

    _parse_format(format, &dict->seps, &dict->fields);	
	dict->dict = _dictionary_load_from_file(filename, dict->seps);
	
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
	
	if ( self->dict == NULL ) {
		printf("No dictionary loaded.\n");
		return;
	}
	if ( strlen(query) == 0 )
		return;
	
	int size = 0;
	while ( self->dict[size] != NULL ) { size++; }
	
	/* Search the dictionary. FIXME: We might want to implement a reasonable search algorithm. */
	if (size == 0) {
		printf("No dictionary loaded.\n");
	}
	else {
		int i, j, k;
		int count = 0;
		for (i = 0; i < size; i++) {
			int isize = 0;
			while ( self->dict[i][isize] != NULL ) { isize++; }
			
			/* Look in all fields for matches */
			for (k = 0; k<isize; k++) {
				if (strstr(self->dict[i][k], query) != NULL ) {
					/* Format and calculate the score */
					int score = strlen(self->dict[i][k]);
					score += k;		// Penalty for being in later fields, usually gives a nice result

                    /* If there are enough better matches, skip this one */
                    Match *last = eina_list_data_get(eina_list_last(result));
                    if (result && score > last->score && count >= self->max_nrof_results) 
                        break;

                    /* Build return value */
                    match = calloc(1, sizeof(Match));
                    match->score = score;
                    match->str = _format_result(self->dict[i], self->fields, self->markup, query);
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
	if ( self->dict == NULL) {
		return 0;
	}
	
	int size = 0;
	while ( self->dict[size] != NULL ) { size++; }
	return size;
}


/* Load the dictionary into memory */
static char *** _dictionary_load_from_file(const char *filename, char **seps)
{
	/* Read file to memory */
	FILE * file_ptr = fopen( filename, "r" );
	if ( file_ptr == NULL )  /* Could not open file */
	{
		printf("Dictionary file not found: %s\n", filename);
		return NULL;
	}
	long  file_length;
	char * file_contents;
	
	/* Get file length */
	fseek( file_ptr, 0L, SEEK_END );
	file_length = ftell( file_ptr );
	rewind( file_ptr );
	
	/* Allocate memory and read entire file to memory */
	file_contents = calloc( file_length + 1, sizeof(char) );
	if ( file_contents == NULL )
	{
		printf( "Insufficient memory to read file.\n" );
		return NULL;
	}
	fread( file_contents, file_length, 1, file_ptr );
	
	/* Parse the dictionary and store it in the global variable dictionary*/	
	/* Get number of lines in the file */
	int nr_lines = 0;
	char * current = file_contents;
	while ( current[0] != '\0' ) {
		if ( current [0] == '\n' ) { nr_lines++; }
		current++;
	}
	printf( "Dictionary size: %d\n", nr_lines );
	
	/* Allocate memory */
	char ***dict = malloc( (nr_lines+1) * sizeof(char **) );
	if ( dict == NULL ) {
		printf( "Insufficient memory to parse dictionary.\n" );
		return NULL;
	}
	
	/* Add entries to the new dictionary */	
	char * line = strtok( file_contents, "\n" );
	int entry_nr = 0;
	while ( line != NULL && entry_nr < nr_lines )
	{
		/* Renove comments and blank lines */
		if ( line[0] != '#' && strlen(line) > 0 ) {
			dict[entry_nr] = _parse_input_line(line, seps);
			//printf("%s\n", dict[entry_nr][0]);
			entry_nr++;
		}
		line = strtok( NULL, "\n" );
	}
	dict[entry_nr] = NULL;
	
	/* Clean up */
	printf( "Dictionary loaded.\n" );
	free( file_contents );
	return dict;
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


/* Parse a line according to the given format and return a NULL-terminated list of strings */
static char ** _parse_input_line(const char *line, char **seps) {
	int i;
	int nr_of_seps = 0;
	if (seps != NULL) {
		while ( seps[nr_of_seps] != NULL ) { nr_of_seps++; }
	}
	
	/* If we have no reasonable format data just return the whole line */
	if (nr_of_seps < 2) { 
		char **result = malloc(2 * sizeof(char *));
		result[0] = strdup(line);
		result[1] = NULL;
		return result;
	}
	
	/* Else break it up into fields */
	char **result = malloc((nr_of_seps) * sizeof(char *));
	char *pos = strstr( line, seps[0] );
	if ( pos == NULL ) {
		// Illegal format, return without parsing
		//~ printf("Illegal, %d: %s\n", i, line);
		free(result);
		result = malloc(2 * sizeof(char *));
		result[0] = strdup(line);
		result[1] = NULL;
		return result;
	}
	pos = pos + strlen(seps[0]);
	
	char *end;
	char *begin;
	/* Parse all but the last field */ 
	for (i = 0; i < nr_of_seps - 2; i++) {
		begin = pos;
		end = strstr( pos, seps[i+1] );
		if ( begin == NULL || end == NULL) {
			// Illegal format, return without parsing
			//~ printf("Illegal, %d: %s\n", i, line);
			free(result);
			result = malloc(2 * sizeof(char *));
			result[0] = strdup(line);
			result[1] = NULL;
			return result;
		}
		int len = end - begin;
		result[i] = malloc( (len+1)*sizeof(char) );
		strncpy( result[i], begin, len);
		result[i][len] = '\0';
		pos = end + strlen(seps[i+1]);
	}

	/* The last field we take the last separator from the end, not the first match */
	begin = pos;
	end = (char *)&line[ strlen(line) - strlen( seps[nr_of_seps-1] ) ];
	if ( strcmp( end, seps[nr_of_seps-1] ) == 0 ) {
		int len = end-begin;
		result[i] = malloc( (len+1)*sizeof(char) );
		strncpy( result[i], begin, len);
		result[i][len] = '\0';	
	} else {
		// Illegal format, return without parsing
		//~ printf("Illegal, last: %s\n", line);
		free(result);
		result = malloc(2 * sizeof(char *));
		result[0] = strdup(line);
		result[1] = NULL;
		return result;
	}
	result[nr_of_seps-1] = NULL;
	
	return result;
}

/* Format result according to given markup string */
static char * _format_result(char **match, char **fields, char *markup, const char *query) {
	int k, j;
    const char * match_tag = "match";

	/* If we have no format or markup just return the first field in match */
	if ( fields == NULL || markup == NULL ) 
		return strdup(match[0]);
	if ( strlen(markup) == 0 ) 
		return strdup(match[0]);
		
	int nr_of_fields = 0;
	while ( fields[nr_of_fields] != NULL ) { nr_of_fields++; }
	
	/* If the match does not contain the given number of fields, just return it as it is */
	int size = 0;
	while ( match[size] != NULL ) { size++; }
	if (size != nr_of_fields)
		return strdup(match[0]);

    /* Count the number of matching substrings */
    int nrof_matches = 0;
	for (k = 0; k < nr_of_fields; k++) {
        char *tmp = strstr(match[k], query);
        while (tmp) {
            nrof_matches++;
            tmp = strstr(tmp + strlen(query), query);
        }
    }
	
	/* Set the length of the returned string as the length of 
     * markup + fields + nrof_matches * tags */
	int len = strlen(markup) + nrof_matches*(5 + 2*strlen(match_tag)) - 2*nr_of_fields;
	for (k = 0; k < nr_of_fields; k++) 
		len = len+strlen(match[k]);
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
                char *begin = match[field];
                char *end = strstr(match[field], query);
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
                    begin = end + strlen(query);
                    end = strstr(end + strlen(query), query);
                }
				strcpy( &result[pos], begin );
				pos += strlen(begin);
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

