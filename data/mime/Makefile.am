# Based On Totem

nodist_noinst_HEADERS = parole-mime-types.h

parole-mime-types.h: mime-type-include.sh mime-type-list.txt mime-functions.sh
	$(srcdir)/mime-type-include.sh $(srcdir)/mime-type-list.txt     \
	> $@

EXTRA_DIST =			\
	mime-functions.sh	\
	mime-type-include.sh	\
	mime-type-list.txt

CLEANFILES =			\
	parole-mime-types.h