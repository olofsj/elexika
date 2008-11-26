MAINTAINERCLEANFILES = Makefile.in
SUBDIRS = images

EDJE_CC = edje_cc
EDJE_FLAGS = \
-v \
-id $(top_srcdir)/data/themes/images \
-fd $(top_srcdir)/data/themes/fonts 

filesdir = $(datadir)/themes
files_DATA = \
elexika.edj

EXTRA_DIST = \
elexika.edc

elexika.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/elexika.edc \
	$(top_builddir)/data/themes/elexika.edj

clean-local:
	rm -f *.edj
