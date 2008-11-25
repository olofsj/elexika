AUTOMAKE_OPTIONS     = 1.4 foreign

MAINTAINERCLEANFILES = Makefile.in

INCLUDES = \
-I$(top_srcdir) \
-I$(top_srcdir)/src/bin \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@OMDICT_CFLAGS@

bin_PROGRAMS = omdict

omdict_INCLUDES = \
omdict_result_list.h \
omdict_dictionary.h

omdict_SOURCES = \
main.c \
omdict_result_list.c \
omdict_dictionary.c

omdict_LDADD = @OMDICT_LIBS@
