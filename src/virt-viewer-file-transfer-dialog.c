/*
 * Virt Viewer: A virtual machine console viewer
 *
 * Copyright (C) 2016 Red Hat, Inc.
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

#include <config.h>

#include "virt-viewer-file-transfer-dialog.h"
#include <glib/gi18n.h>

G_DEFINE_TYPE(VirtViewerFileTransferDialog, virt_viewer_file_transfer_dialog, GTK_TYPE_DIALOG)

#define FILE_TRANSFER_DIALOG_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE((o), VIRT_VIEWER_TYPE_FILE_TRANSFER_DIALOG, VirtViewerFileTransferDialogPrivate))

struct _VirtViewerFileTransferDialogPrivate
{
    /* GHashTable<SpiceFileTransferTask, widgets> */
    GHashTable *file_transfers;
    guint timer_show_src;
    guint timer_hide_src;
};


static void
virt_viewer_file_transfer_dialog_dispose(GObject *object)
{
    VirtViewerFileTransferDialog *self = VIRT_VIEWER_FILE_TRANSFER_DIALOG(object);

    g_clear_pointer(&self->priv->file_transfers, g_hash_table_unref);

    G_OBJECT_CLASS(virt_viewer_file_transfer_dialog_parent_class)->dispose(object);
}

static void
virt_viewer_file_transfer_dialog_class_init(VirtViewerFileTransferDialogClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    g_type_class_add_private(klass, sizeof(VirtViewerFileTransferDialogPrivate));

    object_class->dispose = virt_viewer_file_transfer_dialog_dispose;
}

static void
dialog_response(GtkDialog *dialog,
                gint response_id,
                gpointer user_data G_GNUC_UNUSED)
{
    VirtViewerFileTransferDialog *self = VIRT_VIEWER_FILE_TRANSFER_DIALOG(dialog);
    GHashTableIter iter;
    gpointer key, value;

    switch (response_id) {
        case GTK_RESPONSE_CANCEL:
            /* cancel all current tasks */
            g_hash_table_iter_init(&iter, self->priv->file_transfers);

            while (g_hash_table_iter_next(&iter, &key, &value)) {
                spice_file_transfer_task_cancel(SPICE_FILE_TRANSFER_TASK(key));
            }
            break;
        case GTK_RESPONSE_DELETE_EVENT:
            /* silently ignore */
            break;
        default:
            g_warn_if_reached();
    }
}

static void task_cancel_clicked(GtkButton *button G_GNUC_UNUSED,
                                gpointer user_data)
{
    SpiceFileTransferTask *task = user_data;
    spice_file_transfer_task_cancel(task);
}

typedef struct {
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *progress;
    GtkWidget *label;
    GtkWidget *cancel;
} TaskWidgets;

static TaskWidgets *task_widgets_new(SpiceFileTransferTask *task)
{
    TaskWidgets *w = g_new0(TaskWidgets, 1);

    w->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    w->hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    w->progress = gtk_progress_bar_new();
    w->label = gtk_label_new(spice_file_transfer_task_get_filename(task));
    w->cancel = gtk_button_new_from_icon_name("gtk-cancel", GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_widget_set_hexpand(w->progress, TRUE);
    gtk_widget_set_valign(w->progress, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(w->label, TRUE);
    gtk_widget_set_valign(w->label, GTK_ALIGN_END);
    gtk_widget_set_halign(w->label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(w->cancel, FALSE);
    gtk_widget_set_valign(w->cancel, GTK_ALIGN_CENTER);

    g_signal_connect(w->cancel, "clicked",
                     G_CALLBACK(task_cancel_clicked), task);

    gtk_box_pack_start(GTK_BOX(w->hbox), w->progress, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(w->hbox), w->cancel, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(w->vbox), w->label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(w->vbox), w->hbox, TRUE, TRUE, 0);

    gtk_widget_show_all(w->vbox);
    return w;
}

static gboolean delete_event(GtkWidget *widget,
                             GdkEvent *event G_GNUC_UNUSED,
                             gpointer user_data G_GNUC_UNUSED)
{
    /* don't allow window to be deleted, just process the response signal,
     * which may result in the window being hidden */
    gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_CANCEL);
    return TRUE;
}

static void
virt_viewer_file_transfer_dialog_init(VirtViewerFileTransferDialog *self)
{
    GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(self)));

    self->priv = FILE_TRANSFER_DIALOG_PRIVATE(self);

    gtk_widget_set_size_request(GTK_WIDGET(content), 400, -1);
    gtk_container_set_border_width(GTK_CONTAINER(content), 12);
    self->priv->file_transfers = g_hash_table_new_full(g_direct_hash, g_direct_equal,
                                                       g_object_unref,
                                                       (GDestroyNotify)g_free);
    gtk_dialog_add_button(GTK_DIALOG(self), _("Cancel"), GTK_RESPONSE_CANCEL);
    gtk_dialog_set_default_response(GTK_DIALOG(self),
                                    GTK_RESPONSE_CANCEL);
    g_signal_connect(self, "response", G_CALLBACK(dialog_response), NULL);
    g_signal_connect(self, "delete-event", G_CALLBACK(delete_event), NULL);
}

VirtViewerFileTransferDialog *
virt_viewer_file_transfer_dialog_new(GtkWindow *parent)
{
    return g_object_new(VIRT_VIEWER_TYPE_FILE_TRANSFER_DIALOG,
                        "title", _("File Transfers"),
                        "transient-for", parent,
                        "resizable", FALSE,
                        NULL);
}

static void task_progress_notify(GObject *object,
                                 GParamSpec *pspec G_GNUC_UNUSED,
                                 gpointer user_data)
{
    VirtViewerFileTransferDialog *self = VIRT_VIEWER_FILE_TRANSFER_DIALOG(user_data);
    SpiceFileTransferTask *task = SPICE_FILE_TRANSFER_TASK(object);
    TaskWidgets *w = g_hash_table_lookup(self->priv->file_transfers, task);
    g_return_if_fail(w);

    double pct = spice_file_transfer_task_get_progress(task);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(w->progress), pct);
}

static gboolean hide_transfer_dialog(gpointer data)
{
    VirtViewerFileTransferDialog *self = data;
    gtk_widget_hide(GTK_WIDGET(self));
    gtk_dialog_set_response_sensitive(GTK_DIALOG(self),
                                      GTK_RESPONSE_CANCEL, FALSE);
    self->priv->timer_hide_src = 0;

    return G_SOURCE_REMOVE;
}

typedef struct {
    VirtViewerFileTransferDialog *self;
    TaskWidgets *widgets;
    SpiceFileTransferTask *task;
} TaskFinishedData;

static gboolean task_finished_remove(gpointer user_data)
{
    TaskFinishedData *d = user_data;

    gtk_widget_destroy(d->widgets->vbox);

    g_free(d->widgets);
    g_object_unref(d->task);
    g_free(d);

    return G_SOURCE_REMOVE;
}

static void task_finished(SpiceFileTransferTask *task,
                          GError *error,
                          gpointer user_data)
{
    TaskFinishedData *d;
    VirtViewerFileTransferDialog *self = VIRT_VIEWER_FILE_TRANSFER_DIALOG(user_data);
    TaskWidgets *w = g_hash_table_lookup(self->priv->file_transfers, task);

    if (error && !g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning("File transfer task %p failed: %s", task, error->message);

    g_return_if_fail(w);
    gtk_widget_set_sensitive(w->cancel, FALSE);


    d = g_new0(TaskFinishedData, 1);
    d->self = self;
    d->widgets = w;
    d->task = task;

    g_timeout_add(500, task_finished_remove, d);

    g_hash_table_steal(self->priv->file_transfers, task);

    /* if this is the last transfer, close the dialog */
    if (!g_hash_table_size(d->self->priv->file_transfers)) {
        /* cancel any pending 'show' operations if all tasks complete before
         * the dialog can be shown */
        if (self->priv->timer_show_src) {
            g_source_remove(self->priv->timer_show_src);
            self->priv->timer_show_src = 0;
        }
        self->priv->timer_hide_src = g_timeout_add(500, hide_transfer_dialog,
                                                   d->self);
    }
}

static gboolean show_transfer_dialog_delayed(gpointer user_data)
{
    VirtViewerFileTransferDialog *self = user_data;

    self->priv->timer_show_src = 0;
    gtk_widget_show(GTK_WIDGET(self));

    return G_SOURCE_REMOVE;
}

static void show_transfer_dialog(VirtViewerFileTransferDialog *self)
{
    /* if there's a pending 'hide', cancel it */
    if (self->priv->timer_hide_src) {
        g_source_remove(self->priv->timer_hide_src);
        self->priv->timer_hide_src = 0;
    }

    /* don't show the dialog immediately. For very quick transfers, it doesn't
     * make sense to show a dialog and immediately hide it. But if there's
     * already a pending 'show' operation, don't trigger another one */
    if (self->priv->timer_show_src == 0)
        self->priv->timer_show_src = g_timeout_add(250,
                                                   show_transfer_dialog_delayed,
                                                   self);

    gtk_dialog_set_response_sensitive(GTK_DIALOG(self),
                                      GTK_RESPONSE_CANCEL, TRUE);
}

void virt_viewer_file_transfer_dialog_add_task(VirtViewerFileTransferDialog *self,
                                               SpiceFileTransferTask *task)
{
    GtkBox *content = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(self)));
    TaskWidgets *w = task_widgets_new(task);

    gtk_box_pack_start(content,
                       w->vbox,
                       TRUE, TRUE, 12);
    g_hash_table_insert(self->priv->file_transfers, g_object_ref(task), w);
    g_signal_connect(task, "notify::progress", G_CALLBACK(task_progress_notify), self);
    g_signal_connect(task, "finished", G_CALLBACK(task_finished), self);

    show_transfer_dialog(self);
}
