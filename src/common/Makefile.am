noinst_LTLIBRARIES = 			\
	libparolecommon.la		\
	libparolescreensaver.la

libparolecommon_la_SOURCES =		\
	parole-common.c			\
	parole-common.h			\
	parole-rc-utils.c		\
	parole-rc-utils.h		\
	parole-powermanager.c	\
	parole-powermanager.h

libparolecommon_la_CFLAGS =		\
	$(GTK_CFLAGS)			\
	$(LIBXFCE4UTIL_CFLAGS)

libparolescreensaver_la_SOURCES = 	\
	parole-screensaver.c		\
	parole-screensaver.h

libparolescreensaver_la_CFLAGS =		\
	$(GTK_CFLAGS)				\
	$(LIBXFCE4UTIL_CFLAGS)

libparolescreensaver_la_LIBADD = 		\
	$(LIBXFCE4UTIL_LIBS)
