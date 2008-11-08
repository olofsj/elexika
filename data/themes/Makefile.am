MAINTAINERCLEANFILES = Makefile.in
SUBDIRS = images

EDJE_CC = edje_cc
EDJE_FLAGS = \
-v \
-id $(top_srcdir)/data/themes/images \
-fd $(top_srcdir)/data/themes/fonts 

filesdir = $(datadir)/themes
files_DATA = \
omdict.edj

EXTRA_DIST = \
omdict.edc

omdict.edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/omdict.edc \
	$(top_builddir)/data/themes/omdict.edj

clean-local:
	rm -f *.edj
