AC_INIT([Hildon Control Panel Personalisation],
        [1.0.0],
        [merlijn@wizzup.org], [hildon-control-panel-personalisation])

AM_INIT_AUTOMAKE
LT_INIT([shared disable-static])
AC_CONFIG_FILES(Makefile)

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(LIBHILDON, hildon-1)
AC_SUBST(LIBHILDON_CFLAGS)
AC_SUBST(LIBHILDON_LIBS)

PKG_CHECK_MODULES(LIBDBUS, dbus-1)
AC_SUBST(LIBDBUS_CFLAGS)
AC_SUBST(LIBDBUS_LIBS)

PKG_CHECK_MODULES(X11, x11)
AC_SUBST(X11_CFLAGS)
AC_SUBST(X11_LIBS)

PKG_CHECK_MODULES(GCONF, gconf-2.0)
AC_SUBST(GCONF_CFLAGS)
AC_SUBST(GCONF_LIBS)

hildoncontrolpaneldesktopentrydir="`$PKG_CONFIG --variable=plugindesktopentrydir hildon-control-panel`"
AC_SUBST(hildoncontrolpaneldesktopentrydir)

hildoncontrolpanelpluginlibdir="`$PKG_CONFIG --variable=pluginlibdir hildon-control-panel`"
AC_SUBST(hildoncontrolpanelpluginlibdir)

AC_OUTPUT
