/*
 * fmtx-status-menu-item: Clone of Nokia's fmtx-status-menu-item
 *
 * Copyright (C) 2009 Faheem Pervez <trippin1@gmail.com>. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *      
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *      
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Thanks to simple-fmtx-widget author Pekka Rönkkö, from which some code was taken and used here. 
 *
 */

/* fmtx_status_menu_item.c */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <libintl.h>
#include <locale.h>

#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <libosso.h>
#include <stdlib.h>
#include <dbus/dbus-glib.h>
#include <gconf/gconf-client.h>
#include <gtk/gtk.h>
#include <hildon/hildon.h>
#include <libhildondesktop/libhildondesktop.h>

#define FMTX_CHANGE_SIG "Changed"
#define FMTX_ERROR_SIG "Error"

#define GCONF_DIR "/apps/osso/maemo-statusmenu-fmtx"
#define GCONF_KEY_ALWAYS_VISIBLE "/apps/osso/maemo-statusmenu-fmtx/always_visible"

/* Start of GObject boilerplate shit */

#define FMTX_TYPE_STATUS_MENU_ITEM (fmtx_status_menu_item_get_type ())

#define FMTX_STATUS_MENU_ITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
						FMTX_TYPE_STATUS_MENU_ITEM, FmtxStatusMenuItem))

#define FMTX_STATUS_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
						FMTX_TYPE_STATUS_MENU_ITEM, FmtxStatusMenuItemClass))

#define FMTX_IS_STATUS_MENU_ITEM(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
						FMTX_TYPE_STATUS_MENU_ITEM))

#define FMTX_IS_STATUS_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
						FMTX_TYPE_STATUS_MENU_ITEM))

#define FMTX_STATUS_MENU_ITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
						FMTX_TYPE_STATUS_MENU_ITEM, FmtxStatusMenuItemClass))

#define FMTX_STATUS_MENU_ITEM_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
						FMTX_TYPE_STATUS_MENU_ITEM, FmtxStatusMenuItemPrivate))

#define FMTX_SERVICE "com.nokia.FMTx"
#define FMTX_OBJ_PATH "/com/nokia/fmtx/default"

#define DBUS_PROPERTIES_SET "Set"
#define DBUS_PROPERTIES_GET "Get"

typedef struct _FmtxStatusMenuItem FmtxStatusMenuItem;
typedef struct _FmtxStatusMenuItemClass FmtxStatusMenuItemClass;
typedef struct _FmtxStatusMenuItemPrivate FmtxStatusMenuItemPrivate;

struct _FmtxStatusMenuItem
{
	HDStatusMenuItem parent;
};

struct _FmtxStatusMenuItemClass
{
	HDStatusMenuItemClass parent_class;
};

struct _FmtxStatusMenuItemPrivate
{
	osso_context_t *osso_context;
	DBusGConnection *sys_dbus_conn;
	DBusGProxy *fmtx_properties_proxy;
	DBusGProxy *fmtx_proxy;
	GtkWidget *fmtx_button;
        gboolean is_fmtx_enabled;
	GConfClient *gconf_client;
};
/* End of GObject boilerplate shit */

HD_DEFINE_PLUGIN_MODULE (FmtxStatusMenuItem, fmtx_status_menu_item, HD_TYPE_STATUS_MENU_ITEM)

static gboolean fmtx_status_menu_item_load_control_panel_applet (FmtxStatusMenuItem *plugin)
{
	FmtxStatusMenuItemPrivate *priv = FMTX_STATUS_MENU_ITEM_GET_PRIVATE (plugin);

	osso_cp_plugin_execute (priv->osso_context, "libcpfmtx.so", plugin, TRUE);

	return FALSE;
}

static void fmtx_status_menu_item_on_button_clicked (GtkWidget *button, FmtxStatusMenuItem *plugin)
{
	/* g_timeout_add (300, (GSourceFunc) fmtx_status_menu_item_load_control_panel_applet, plugin); */ /* Possible race condition, anyone? */

	/* A better way: */
	/* gtk_widget_hide (gtk_widget_get_toplevel (button)); */ /* An even better way is using g_signal_connect_after() */
	(void) fmtx_status_menu_item_load_control_panel_applet (plugin);
}

static void fmtx_status_menu_item_on_fmtx_error_caught (DBusGProxy *object G_GNUC_UNUSED, const gchar *error, FmtxStatusMenuItem *plugin)
{
	/* FmtxStatusMenuItemPrivate *priv = FMTX_STATUS_MENU_ITEM_GET_PRIVATE (plugin); */

	hildon_banner_show_information (GTK_WIDGET (plugin), NULL, dcgettext ("osso-fm-transmitter", error, LC_MESSAGES));
}

static void fmtx_status_menu_item_on_fmtx_property_changed (DBusGProxy *object G_GNUC_UNUSED, FmtxStatusMenuItem *plugin)
{
	GError *error = NULL;
	GValue freq_value = { 0 };
	gchar freq_value_text[16];
	gchar button_value_text[128];
        gchar button_value_off_text[32];
        GValue state_value = { 0 };
	FmtxStatusMenuItemPrivate *priv = FMTX_STATUS_MENU_ITEM_GET_PRIVATE (plugin);

	dbus_g_proxy_call (priv->fmtx_properties_proxy, "Get", &error, G_TYPE_STRING, "org.freedesktop.DBus.Properties", G_TYPE_STRING, "frequency", G_TYPE_INVALID, G_TYPE_VALUE, &freq_value, G_TYPE_INVALID);
	if (error)
	{
		g_clear_error (&error);
		return;
	}

	g_snprintf (freq_value_text, sizeof (freq_value_text), "%.1f", (gdouble) g_value_get_uint (&freq_value)/1000);
	g_snprintf (button_value_text, sizeof (button_value_text), dcgettext ("osso-statusbar-sound", "stab_va_sound_fm_frequency", LC_MESSAGES), freq_value_text); /* Anyone knows how to do this properly, so that I don't get "warning: too many arguments for format"? - qwerty12 */
        g_snprintf (button_value_off_text, sizeof (button_value_off_text), dcgettext ("osso-connectivity-ui", "stab_me_bt_off", LC_MESSAGES));
        g_value_unset (&freq_value);

	dbus_g_proxy_call (priv->fmtx_properties_proxy, "Get", &error, G_TYPE_STRING, "org.freedesktop.DBus.Properties", G_TYPE_STRING, "state", G_TYPE_INVALID, G_TYPE_VALUE, &state_value, G_TYPE_INVALID);
	if (error)
	{
		g_clear_error (&error);
		return;
	}
	
	if (g_str_equal (g_value_get_string (&state_value), "enabled"))
	{
		GdkPixbuf *fm_status_area_pixbuf = NULL;

		gtk_widget_show (GTK_WIDGET (plugin));
		gtk_widget_show (priv->fmtx_button);
		fm_status_area_pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(), "general_fm_transmitter", 18, 0, NULL);
		hd_status_plugin_item_set_status_area_icon (HD_STATUS_PLUGIN_ITEM (plugin), fm_status_area_pixbuf);
		hildon_button_set_value (HILDON_BUTTON (priv->fmtx_button), button_value_text);
		priv->is_fmtx_enabled = TRUE;
		g_object_unref (fm_status_area_pixbuf); /* The Pixbuf needs to be unreffed. - qwerty12*/
	}
	else
	{
		if (!gconf_client_get_bool (priv->gconf_client, GCONF_KEY_ALWAYS_VISIBLE, NULL))
			gtk_widget_hide (GTK_WIDGET (plugin));
                hildon_button_set_value (HILDON_BUTTON (priv->fmtx_button), button_value_off_text);
		priv->is_fmtx_enabled = FALSE;
		hd_status_plugin_item_set_status_area_icon (HD_STATUS_PLUGIN_ITEM (plugin), NULL);
	}

	g_value_unset (&state_value); /* The string GValue also needs to be freed, too. - qwerty12*/
}

static void fmtx_status_menu_on_tap_and_hold (GtkWidget *widget, FmtxStatusMenuItem *plugin)
{
	FmtxStatusMenuItemPrivate *priv = FMTX_STATUS_MENU_ITEM_GET_PRIVATE (plugin);
	GError *error = NULL;
	GValue new_state = { 0 };

	gtk_widget_hide (gtk_widget_get_toplevel (widget));
	g_value_init(&new_state, G_TYPE_STRING);

	if (priv->is_fmtx_enabled)
		g_value_set_static_string(&new_state, "disabled");
	else
		g_value_set_static_string(&new_state, "enabled");

	dbus_g_proxy_call (priv->fmtx_properties_proxy, DBUS_PROPERTIES_SET, &error, G_TYPE_STRING,
	                   "org.freedesktop.DBus.Properties", G_TYPE_STRING, "state",
	                   G_TYPE_VALUE, &new_state, G_TYPE_INVALID, G_TYPE_INVALID);

	g_value_unset (&new_state);

	if (error)
	{
		g_clear_error (&error);
		return;
	}
}

static void fmtx_status_menu_item_gconf_value_changed (GConfClient *client,
                                			guint cnxn_id,
			                                GConfEntry *entry,
                                			gpointer plugin)
{
	FmtxStatusMenuItemPrivate *priv = FMTX_STATUS_MENU_ITEM_GET_PRIVATE (plugin);

	gboolean is_always_visible = gconf_client_get_bool (priv->gconf_client, GCONF_KEY_ALWAYS_VISIBLE, NULL);

	if (is_always_visible)
	{
		gtk_widget_show_all (GTK_WIDGET (plugin));
	}
	else
	{
		if (!priv->is_fmtx_enabled)
			gtk_widget_hide (GTK_WIDGET (plugin));
	}

}

static void fmtx_status_menu_item_finalize (GObject *object)
{
	FmtxStatusMenuItem *plugin = FMTX_STATUS_MENU_ITEM (object);
	FmtxStatusMenuItemPrivate *priv = FMTX_STATUS_MENU_ITEM_GET_PRIVATE (plugin);

	if (priv->fmtx_proxy)
	{
		dbus_g_proxy_disconnect_signal (priv->fmtx_proxy, FMTX_ERROR_SIG, G_CALLBACK (fmtx_status_menu_item_on_fmtx_error_caught), plugin);
		dbus_g_proxy_disconnect_signal (priv->fmtx_proxy, FMTX_CHANGE_SIG, G_CALLBACK (fmtx_status_menu_item_on_fmtx_property_changed), plugin);
		g_object_unref (G_OBJECT (priv->fmtx_proxy));
	}

	if (priv->fmtx_properties_proxy)
		g_object_unref (G_OBJECT (priv->fmtx_properties_proxy));

	/* if (priv->sys_dbus_conn)
		dbus_g_connection_unref (priv->sys_dbus_conn); */ /* Allegedly, hd_status_plugin_item_get_dbus_g_connection () returns a shared connection. */

	if (priv->osso_context)
		osso_deinitialize (priv->osso_context);

	if (priv->gconf_client)
	{
		g_object_unref (priv->gconf_client);
		priv->gconf_client = NULL;
	}

	G_OBJECT_CLASS (fmtx_status_menu_item_parent_class)->finalize (object);
}

static void fmtx_status_menu_item_class_init (FmtxStatusMenuItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = (GObjectFinalizeFunc) fmtx_status_menu_item_finalize;

	g_type_class_add_private (klass, sizeof (FmtxStatusMenuItemPrivate));
}

static void fmtx_status_menu_item_class_finalize (FmtxStatusMenuItemClass *klass G_GNUC_UNUSED)
{
}

static void fmtx_status_menu_item_init (FmtxStatusMenuItem *plugin)
{
	GError *error = NULL;
        gchar button_value_off_text[32];
	FmtxStatusMenuItemPrivate *priv = FMTX_STATUS_MENU_ITEM_GET_PRIVATE (plugin);

        g_snprintf (button_value_off_text, sizeof (button_value_off_text), dcgettext ("osso-connectivity-ui", "stab_me_bt_off", LC_MESSAGES));

	bindtextdomain ("osso-statusbar-sound", "/usr/share/locale");

	priv->osso_context = osso_initialize ("statusmenu-fmtx", "1", TRUE, NULL);

	priv->gconf_client = gconf_client_get_default ();
	priv->is_fmtx_enabled = FALSE;

	/* priv->sys_dbus_conn = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error); */ /* hd_status_plugin_item_get_dbus_g_connection () is preferred to dbus_g_bus_get () - qwerty12*/
	priv->sys_dbus_conn = hd_status_plugin_item_get_dbus_g_connection (HD_STATUS_PLUGIN_ITEM (&plugin->parent), DBUS_BUS_SYSTEM, &error);
	if (error)
	{
		g_error_free (error);
		return;
	}

	priv->fmtx_properties_proxy = dbus_g_proxy_new_for_name (priv->sys_dbus_conn, "com.nokia.FMTx", "/com/nokia/fmtx/default", "org.freedesktop.DBus.Properties");
	if (!priv->fmtx_properties_proxy)
		return;

	priv->fmtx_proxy = dbus_g_proxy_new_for_name (priv->sys_dbus_conn, "com.nokia.FMTx", "/com/nokia/fmtx/default", "com.nokia.FMTx.Device");
	if (!priv->fmtx_proxy)
		return;

	dbus_g_proxy_add_signal (priv->fmtx_proxy, FMTX_CHANGE_SIG, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->fmtx_proxy, FMTX_CHANGE_SIG, G_CALLBACK (fmtx_status_menu_item_on_fmtx_property_changed), plugin, NULL);
	dbus_g_proxy_add_signal (priv->fmtx_proxy, FMTX_ERROR_SIG, G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->fmtx_proxy, FMTX_ERROR_SIG, G_CALLBACK (fmtx_status_menu_item_on_fmtx_error_caught), plugin, NULL);

	priv->fmtx_button = hildon_button_new (HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH, HILDON_BUTTON_ARRANGEMENT_VERTICAL);
	/* g_signal_connect (priv->fmtx_button, "clicked", G_CALLBACK (fmtx_status_menu_item_on_button_clicked), plugin); */ /* Using g_signal_connect_data () means that the menu wont hide when the button is pressed -qwerty12 */
	g_signal_connect_after (priv->fmtx_button, "clicked", G_CALLBACK (fmtx_status_menu_item_on_button_clicked), plugin); /* Using g_signal_connect_after () means that our handler for bringing up the Control Panel applet will be invoked after the default handler, which hides the menu for us, automatically. - qwerty12 */
        gtk_widget_tap_and_hold_setup (priv->fmtx_button, NULL, NULL, GTK_TAP_AND_HOLD_NONE);
        g_signal_connect (priv->fmtx_button, "tap-and-hold", G_CALLBACK (fmtx_status_menu_on_tap_and_hold), plugin);

	gconf_client_add_dir (priv->gconf_client, GCONF_DIR,
			      GCONF_CLIENT_PRELOAD_NONE, NULL);

	gconf_client_notify_add (priv->gconf_client, GCONF_KEY_ALWAYS_VISIBLE,
				 fmtx_status_menu_item_gconf_value_changed,
				 (gpointer) plugin,
				 NULL, NULL);
	hildon_button_set_style (HILDON_BUTTON (priv->fmtx_button), HILDON_BUTTON_STYLE_PICKER);
        hildon_button_set_title (HILDON_BUTTON (priv->fmtx_button), dcgettext ("osso-statusbar-sound", "stab_me_sound_fm_transmitter", LC_MESSAGES));
	hildon_button_set_image (HILDON_BUTTON (priv->fmtx_button), gtk_image_new_from_icon_name ("general_fm_transmitter", GTK_ICON_SIZE_DIALOG));
	gtk_button_set_alignment (GTK_BUTTON (priv->fmtx_button), 0, 0);
	gtk_container_add (GTK_CONTAINER (plugin), priv->fmtx_button);

	priv->is_fmtx_enabled = gconf_client_get_bool (priv->gconf_client, "/system/fmtx/enabled", NULL);

	if (gconf_client_get_bool (priv->gconf_client, GCONF_KEY_ALWAYS_VISIBLE, NULL))
		gtk_widget_show_all (GTK_WIDGET (plugin)); /* I want the button shown all the time. -qwerty12 */

	if (priv->is_fmtx_enabled)
		fmtx_status_menu_item_on_fmtx_property_changed (NULL, plugin);
        else
                hildon_button_set_value (HILDON_BUTTON (priv->fmtx_button), button_value_off_text);
}
