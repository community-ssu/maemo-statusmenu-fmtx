AC_PREREQ[(2.58)]
AC_INIT(m4_esyscmd([dpkg-parsechangelog | sed -n 's/Source: //p' | tr -d '\n']), m4_esyscmd([dpkg-parsechangelog | sed -n 's/Version: //p' | tr -d '\n']), m4_esyscmd([dpkg-parsechangelog | sed -n 's/Maintainer: //p' | cut -d\< -f2 | sed s/\>// | tr -d '\n']))
AC_CONFIG_SRCDIR([src/fmtx_status_menu_item.c])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER([config.h])

AC_PROG_CC
AM_PROG_CC_C_O
AC_HEADER_STDC
AM_PATH_GLIB_2_0([2.16.0])
AM_PATH_GTK_2_0([2.14.7])
AC_PROG_INSTALL
AM_PROG_LIBTOOL
AM_GLIB_GNU_GETTEXT

PKG_CHECK_MODULES([FMTXSTATUSMENUITEM], [glib-2.0 libosso gobject-2.0 gconf-2.0 dbus-glib-1 gtk+-2.0 hildon-1 libhildondesktop-1])
AC_SUBST([FMTXSTATUSMENUITEM_CFLAGS])
AC_SUBST([FMTXSTATUSMENUITEM_LIBS])

HILDON_DESKTOP_LIB_DIR=`pkg-config libhildondesktop-1 --variable=hildondesktoplibdir`
AC_SUBST([HILDON_DESKTOP_LIB_DIR])

HILDON_HOME_DESKTOP_DIR=`pkg-config libhildondesktop-1 --variable=hildonstatusmenudesktopentrydir`
AC_SUBST(HILDON_HOME_DESKTOP_DIR)

AC_OUTPUT([data/Makefile src/Makefile Makefile])
