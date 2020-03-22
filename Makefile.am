# XXX/FIXME/TODO: honour debian cflags too
CFLAGS = -Wall -Werror -Wextra -std=gnu99 -O2 -ggdb -g
# CFLAGS += $(CFLAGS) -ggdb -g

bin_PROGRAMS = personalisation personalisation_app

personalisation_CFLAGS = $(CFLAGS)
personalisation_SOURCES = personalisation.c

personalisation_app_CFLAGS = $(CFLAGS) $(LIBHILDON_CFLAGS) $(LIBDBUS_CFLAGS) $(X11_CFLAGS) $(GCONF_CFLAGS)
personalisation_app_LDADD = $(LIBHILDON_LIBS) $(LIBDBUS_LIBS) $(X11_LIBS) $(GCONF_LIBS)
personalisation_app_SOURCES = personalisation_app.c

hildoncontrolpanelpluginlib_LTLIBRARIES = libpersonalisation.la

libpersonalisation_la_SOURCES = libpersonalisation.c
libpersonalisation_la_CFLAGS = $(CFLAGS) $(LIBHILDON_CFLAGS) $(LIBDBUS_CFLAGS)
libpersonalisation_la_LIBADD = $(LIBHILDON_LIBS) $(LIBDBUS_LIBS)
libpersonalisation_la_LDFLAGS = -Wl,--as-needed -module -avoid-version

hildoncontrolpaneldesktopentry_DATA = personalisation.desktop

sudoersdir = $(sysconfdir)/sudoers.d/
sudoers_DATA = hildon-control-panel-personalisation.sudoers

EXTRA_DIST = $(hildoncontrolpaneldesktopentry_DATA)		\
             hildon-control-panel-personalisation.sudoers

install-data-hook:
	@(cd $(DESTDIR)$(hildoncontrolpanelplugindir) && $(RM) \
		$(hildoncontrolpanelpluginlib_LTLIBRARIES))
