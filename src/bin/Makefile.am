AUTOMAKE_OPTIONS     = 1.4 foreign

MAINTAINERCLEANFILES = Makefile.in

INCLUDES = \
-I$(top_srcdir) \
-I$(top_srcdir)/src/bin \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@OMDICT_CFLAGS@

bin_PROGRAMS = elexika

elexika_INCLUDES = \
elexika_result_list.h \
elexika_dictionary.h

elexika_SOURCES = \
main.c \
elexika_result_list.c \
elexika_dictionary.c

elexika_LDADD = @OMDICT_LIBS@
