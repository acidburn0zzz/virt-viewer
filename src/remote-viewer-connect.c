/*
 * Virt Viewer: A virtual machine console viewer
 *
 * Copyright (C) 2015 Red Hat, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "remote-viewer-connect.h"
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

typedef struct
{
    gboolean response;
    GMainLoop *loop;
} ConnectionInfo;

static void
shutdown_loop(GMainLoop *loop)
{
    if (g_main_loop_is_running(loop))
        g_main_loop_quit(loop);
}

static gboolean
window_deleted_cb(ConnectionInfo *ci)
{
    ci->response = FALSE;
    shutdown_loop(ci->loop);
    return TRUE;
}

static gboolean
key_pressed_cb(GtkWidget *widget G_GNUC_UNUSED, GdkEvent *event, gpointer data)
{
    GtkWidget *window = data;
    gboolean retval;
    if (event->type == GDK_KEY_PRESS) {
        switch (event->key.keyval) {
            case GDK_KEY_Escape:
                g_signal_emit_by_name(window, "delete-event", NULL, &retval);
                return TRUE;
            default:
                return FALSE;
        }
    }

    return FALSE;
}

static void
connect_button_clicked_cb(GtkButton *button G_GNUC_UNUSED, gpointer data)
{
    ConnectionInfo *ci = data;
    ci->response = TRUE;
    shutdown_loop(ci->loop);
}

static void
connect_dialog_run(ConnectionInfo *ci)
{
    ci->loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(ci->loop);
}

static void
entry_icon_release_cb(GtkEntry* entry, gpointer data G_GNUC_UNUSED)
{
    gtk_entry_set_text(entry, "");
    gtk_widget_grab_focus(GTK_WIDGET(entry));
}

static void
entry_changed_cb(GtkEditable* entry, gpointer data G_GNUC_UNUSED)
{
    gboolean rtl = (gtk_widget_get_direction(GTK_WIDGET(entry)) == GTK_TEXT_DIR_RTL);
    gboolean active = (gtk_entry_get_text_length(GTK_ENTRY(entry)) > 0);

    g_object_set(entry,
                 "secondary-icon-name", active ? (rtl ? "edit-clear-rtl-symbolic" : "edit-clear-symbolic") : NULL,
                 "secondary-icon-activatable", active,
                 "secondary-icon-sensitive", active,
                 NULL);
}

static void
entry_activated_cb(GtkEntry *entry G_GNUC_UNUSED, gpointer data)
{
    ConnectionInfo *ci = data;
    ci->response = TRUE;
    shutdown_loop(ci->loop);
}

static void
recent_selection_changed_dialog_cb(GtkRecentChooser *chooser, gpointer data)
{
    GtkRecentInfo *info;
    GtkWidget *entry = data;
    const gchar *uri;

    info = gtk_recent_chooser_get_current_item(chooser);
    if (info == NULL)
        return;

    uri = gtk_recent_info_get_uri(info);
    g_return_if_fail(uri != NULL);

    gtk_entry_set_text(GTK_ENTRY(entry), uri);

    gtk_recent_info_unref(info);
}

static void
recent_item_activated_dialog_cb(GtkRecentChooser *chooser G_GNUC_UNUSED, gpointer data)
{
    ConnectionInfo *ci = data;
    ci->response = TRUE;
    shutdown_loop(ci->loop);
}

static void
make_label_light(GtkLabel* label)
{
    PangoAttrList* attributes = pango_attr_list_new();
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(label)), "dim-label");
#else
    GtkStyle* style = gtk_widget_get_style(GTK_WIDGET(label));
    GdkColor* c = &style->text[GTK_STATE_INSENSITIVE];
    pango_attr_list_insert(attributes, pango_attr_foreground_new(c->red, c->green, c->blue));
#endif
    pango_attr_list_insert(attributes, pango_attr_scale_new(0.9));
    gtk_label_set_attributes(label, attributes);
    pango_attr_list_unref(attributes);
}

static void
make_label_bold(GtkLabel* label)
{
    PangoAttrList* attributes = pango_attr_list_new();
    pango_attr_list_insert(attributes, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(label, attributes);
    pango_attr_list_unref(attributes);
}

/**
* remote_viewer_connect_dialog
*
* @brief Opens connect dialog for remote viewer
*
* @param uri For returning the uri of chosen server, must be NULL
*
* @return TRUE if Connect or ENTER is pressed
* @return FALSE if Cancel is pressed or dialog is closed
*/
gboolean
remote_viewer_connect_dialog(gchar **uri)
{
    GtkWidget *window, *box, *label, *entry, *recent, *connect_button, *cancel_button, *button_box;
#if !GTK_CHECK_VERSION(3, 0, 0)
    GtkWidget *alignment;
#endif
    GtkRecentFilter *rfilter;

    ConnectionInfo ci = {
        FALSE,
        NULL
    };

    g_return_val_if_fail(uri && *uri == NULL, FALSE);

    /* Create the widgets */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_container_set_border_width(GTK_CONTAINER(window), 5);
    box = gtk_vbox_new(FALSE, 6);
    gtk_container_set_border_width(GTK_CONTAINER(box), 5);
    gtk_container_add(GTK_CONTAINER(window), box);

    label = gtk_label_new_with_mnemonic(_("_Connection Address"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    entry = GTK_WIDGET(gtk_entry_new());
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    g_object_set(entry, "width-request", 200, NULL);
    g_signal_connect(entry, "changed", G_CALLBACK(entry_changed_cb), entry);
    g_signal_connect(entry, "icon-release", G_CALLBACK(entry_icon_release_cb), entry);
    gtk_box_pack_start(GTK_BOX(box), entry, TRUE, TRUE, 0);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);
    make_label_bold(GTK_LABEL(label));

    label = gtk_label_new(_("For example, spice://foo.example.org:5900"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    make_label_light(GTK_LABEL(label));
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    gtk_widget_set_margin_bottom(label, 12);
#else
    alignment = gtk_alignment_new(0, 0, 1, 1);
    gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 12, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), label);
    gtk_box_pack_start(GTK_BOX(box), alignment, TRUE, TRUE, 0);
#endif

    label = gtk_label_new_with_mnemonic(_("_Recent Connections"));
    make_label_bold(GTK_LABEL(label));
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);

    recent = GTK_WIDGET(gtk_recent_chooser_widget_new());
    gtk_recent_chooser_set_show_icons(GTK_RECENT_CHOOSER(recent), FALSE);
    gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER(recent), GTK_RECENT_SORT_MRU);
    gtk_box_pack_start(GTK_BOX(box), recent, TRUE, TRUE, 0);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), recent);

    rfilter = gtk_recent_filter_new();
    gtk_recent_filter_add_mime_type(rfilter, "application/x-spice");
    gtk_recent_filter_add_mime_type(rfilter, "application/x-vnc");
    gtk_recent_filter_add_mime_type(rfilter, "application/x-virt-viewer");
    gtk_recent_chooser_set_filter(GTK_RECENT_CHOOSER(recent), rfilter);
    gtk_recent_chooser_set_local_only(GTK_RECENT_CHOOSER(recent), FALSE);

    button_box = gtk_hbutton_box_new();
    gtk_button_box_set_layout(GTK_BUTTON_BOX(button_box), GTK_BUTTONBOX_END);
    connect_button = gtk_button_new_with_label("Connect");
    cancel_button = gtk_button_new_with_label("Cancel");
    gtk_box_pack_start(GTK_BOX(button_box), cancel_button, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), connect_button, FALSE, TRUE, 1);

    gtk_box_pack_start(GTK_BOX(box), button_box, FALSE, TRUE, 0);

    g_signal_connect(window, "key-press-event",
                     G_CALLBACK(key_pressed_cb), window);
    g_signal_connect(connect_button, "clicked",
                     G_CALLBACK(connect_button_clicked_cb), &ci);

    /* make sure that user_data is passed as first parameter */
    g_signal_connect_swapped(cancel_button, "clicked",
                             G_CALLBACK(window_deleted_cb), &ci);
    g_signal_connect_swapped(window, "delete-event",
                             G_CALLBACK(window_deleted_cb), &ci);

    g_signal_connect(entry, "activate",
                     G_CALLBACK(entry_activated_cb), &ci);
    g_signal_connect(entry, "changed",
                     G_CALLBACK(entry_changed_cb), connect_button);
    g_signal_connect(entry, "icon-release",
                     G_CALLBACK(entry_icon_release_cb), entry);

    g_signal_connect(recent, "selection-changed",
                     G_CALLBACK(recent_selection_changed_dialog_cb), entry);
    g_signal_connect(recent, "item-activated",
                     G_CALLBACK(recent_item_activated_dialog_cb), &ci);

    /* show and wait for response */
    gtk_widget_show_all(window);

    connect_dialog_run(&ci);
    if (ci.response == TRUE) {
        *uri = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
        g_strstrip(*uri);
    } else {
        *uri = NULL;
    }

    gtk_widget_destroy(window);

    return ci.response;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 * End:
 */
