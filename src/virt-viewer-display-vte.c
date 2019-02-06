/*
 * Virt Viewer: A virtual machine console viewer
 *
 * Copyright (C) 2018 Red Hat, Inc.
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
 *
 * Author: Marc-André Lureau <marcandre.lureau@redhat.com>
 */

#include <config.h>
#include <glib/gi18n.h>

#ifdef HAVE_VTE
#include <vte/vte.h>
#endif

#include "virt-viewer-auth.h"
#include "virt-viewer-display-vte.h"
#include "virt-viewer-util.h"

struct _VirtViewerDisplayVtePrivate {
#ifdef HAVE_VTE
    VteTerminal *vte;
#endif
    GtkWidget *scroll;
    gchar *name;
};

G_DEFINE_TYPE_WITH_PRIVATE(VirtViewerDisplayVte, virt_viewer_display_vte, VIRT_VIEWER_TYPE_DISPLAY)

enum {
    PROP_0,

    PROP_NAME,
};

static void
virt_viewer_display_vte_finalize(GObject *obj)
{
    G_OBJECT_CLASS(virt_viewer_display_vte_parent_class)->finalize(obj);
}

static void
virt_viewer_display_vte_set_property(GObject *object,
                                     guint prop_id,
                                     const GValue *value,
                                     GParamSpec *pspec)
{
    VirtViewerDisplayVte *self = VIRT_VIEWER_DISPLAY_VTE(object);

    switch (prop_id) {
    case PROP_NAME:
        g_free(self->priv->name);
        self->priv->name = g_value_dup_string(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
virt_viewer_display_vte_get_property(GObject *object,
                                     guint prop_id,
                                     GValue *value,
                                     GParamSpec *pspec)
{
    VirtViewerDisplayVte *self = VIRT_VIEWER_DISPLAY_VTE(object);

    switch (prop_id) {
    case PROP_NAME:
        g_value_set_string(value, self->priv->name);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
virt_viewer_display_vte_size_allocate(GtkWidget *widget G_GNUC_UNUSED,
                                      GtkAllocation *allocation G_GNUC_UNUSED)
{
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(widget));

    if (child && gtk_widget_get_visible(child))
        gtk_widget_size_allocate(child, allocation);
}

static void
virt_viewer_display_vte_class_init(VirtViewerDisplayVteClass *klass)
{
    GObjectClass *oclass = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    oclass->set_property = virt_viewer_display_vte_set_property;
    oclass->get_property = virt_viewer_display_vte_get_property;
    oclass->finalize = virt_viewer_display_vte_finalize;
    /* override display desktop aspect-ratio behaviour */
    widget_class->size_allocate = virt_viewer_display_vte_size_allocate;

    g_object_class_install_property(oclass,
                                    PROP_NAME,
                                    g_param_spec_string("name",
                                                        "Name",
                                                        "Console name",
                                                        NULL,
                                                        G_PARAM_READWRITE|
                                                        G_PARAM_CONSTRUCT_ONLY|
                                                        G_PARAM_STATIC_STRINGS));
    g_signal_new("commit",
                 G_OBJECT_CLASS_TYPE(oclass),
                 G_SIGNAL_RUN_FIRST,
                 0,
                 NULL, NULL,
                 NULL,
                 G_TYPE_NONE,
                 2,
                 G_TYPE_POINTER, G_TYPE_INT);
}

static void
virt_viewer_display_vte_init(VirtViewerDisplayVte *self G_GNUC_UNUSED)
{
    self->priv = virt_viewer_display_vte_get_instance_private(self);
}

#ifdef HAVE_VTE
static void
virt_viewer_display_vte_commit(VirtViewerDisplayVte *self,
                               const gchar *text,
                               guint size,
                               gpointer user_data G_GNUC_UNUSED)
{
    g_signal_emit_by_name(self, "commit", text, size);
}
#endif

static void
virt_viewer_display_vte_adj_changed(VirtViewerDisplayVte *self,
                                    GtkAdjustment *adjustment)
{
    gtk_widget_set_visible(self->priv->scroll,
        gtk_adjustment_get_upper(adjustment) > gtk_adjustment_get_page_size(adjustment));
}

GtkWidget *
virt_viewer_display_vte_new(VirtViewerSession *session, const char *name)
{
    VirtViewerDisplayVte *self;
    GtkWidget *grid, *scroll = NULL, *vte;

    self = g_object_new(VIRT_VIEWER_TYPE_DISPLAY_VTE,
                        "session", session,
                        "nth-display", -1,
                        "name", name,
                        NULL);
#ifdef HAVE_VTE
    vte = vte_terminal_new();
    self->priv->vte = VTE_TERMINAL(g_object_ref(vte));
    virt_viewer_signal_connect_object(vte, "commit",
                                      G_CALLBACK(virt_viewer_display_vte_commit),
                                      self, G_CONNECT_SWAPPED);
    scroll = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL,
                               gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(vte)));
    self->priv->scroll = scroll;
#else
    vte = gtk_label_new(_("Console support is compiled out!"));
#endif
    g_object_set(vte, "hexpand", TRUE, "vexpand", TRUE, NULL);

    grid = gtk_grid_new();

    gtk_container_add(GTK_CONTAINER(grid), vte);
    if (scroll) {
        gtk_container_add(GTK_CONTAINER(grid), scroll);
        gtk_widget_hide(scroll);
        virt_viewer_signal_connect_object(gtk_range_get_adjustment(GTK_RANGE(scroll)),
                                          "changed", G_CALLBACK(virt_viewer_display_vte_adj_changed),
                                          self, G_CONNECT_SWAPPED);
    }

    gtk_container_add(GTK_CONTAINER(self), grid);

    return GTK_WIDGET(self);
}

/* adapted from gnome-terminal */
/* Allow scales a bit smaller and a bit larger than the usual pango ranges */
#define TERMINAL_SCALE_XXX_SMALL   (PANGO_SCALE_XX_SMALL/1.2)
#define TERMINAL_SCALE_XXXX_SMALL  (TERMINAL_SCALE_XXX_SMALL/1.2)
#define TERMINAL_SCALE_XXXXX_SMALL (TERMINAL_SCALE_XXXX_SMALL/1.2)
#define TERMINAL_SCALE_XXX_LARGE   (PANGO_SCALE_XX_LARGE*1.2)
#define TERMINAL_SCALE_XXXX_LARGE  (TERMINAL_SCALE_XXX_LARGE*1.2)
#define TERMINAL_SCALE_XXXXX_LARGE (TERMINAL_SCALE_XXXX_LARGE*1.2)
#define TERMINAL_SCALE_MINIMUM     (TERMINAL_SCALE_XXXXX_SMALL/1.2)
#define TERMINAL_SCALE_MAXIMUM     (TERMINAL_SCALE_XXXXX_LARGE*1.2)

#ifdef HAVE_VTE
static const double zoom_factors[] = {
  TERMINAL_SCALE_MINIMUM,
  TERMINAL_SCALE_XXXXX_SMALL,
  TERMINAL_SCALE_XXXX_SMALL,
  TERMINAL_SCALE_XXX_SMALL,
  PANGO_SCALE_XX_SMALL,
  PANGO_SCALE_X_SMALL,
  PANGO_SCALE_SMALL,
  PANGO_SCALE_MEDIUM,
  PANGO_SCALE_LARGE,
  PANGO_SCALE_X_LARGE,
  PANGO_SCALE_XX_LARGE,
  TERMINAL_SCALE_XXX_LARGE,
  TERMINAL_SCALE_XXXX_LARGE,
  TERMINAL_SCALE_XXXXX_LARGE,
  TERMINAL_SCALE_MAXIMUM
};

static gboolean
find_larger_zoom_factor (double *zoom)
{
  double current = *zoom;
  guint i;

  for (i = 0; i < G_N_ELEMENTS (zoom_factors); ++i)
    {
      /* Find a font that's larger than this one */
      if ((zoom_factors[i] - current) > 1e-6)
        {
          *zoom = zoom_factors[i];
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
find_smaller_zoom_factor (double *zoom)
{
  double current = *zoom;
  int i;

  i = (int) G_N_ELEMENTS (zoom_factors) - 1;
  while (i >= 0)
    {
      /* Find a font that's smaller than this one */
      if ((current - zoom_factors[i]) > 1e-6)
        {
          *zoom = zoom_factors[i];
          return TRUE;
        }

      --i;
    }

  return FALSE;
}

void virt_viewer_display_vte_feed(VirtViewerDisplayVte *display, gpointer data, int size)
{
    vte_terminal_feed(display->priv->vte, data, size);
}

void virt_viewer_display_vte_zoom_in(VirtViewerDisplayVte *self)
{
    double zoom = vte_terminal_get_font_scale(self->priv->vte);

    if (!find_larger_zoom_factor(&zoom))
        return;

    vte_terminal_set_font_scale(self->priv->vte, zoom);
}

void virt_viewer_display_vte_zoom_out(VirtViewerDisplayVte *self)
{
    double zoom = vte_terminal_get_font_scale(self->priv->vte);

    if (!find_smaller_zoom_factor(&zoom))
        return;

    vte_terminal_set_font_scale(self->priv->vte, zoom);
}

void virt_viewer_display_vte_zoom_reset(VirtViewerDisplayVte *self)
{
    vte_terminal_set_font_scale(self->priv->vte, PANGO_SCALE_MEDIUM);
}
#else
void virt_viewer_display_vte_feed(VirtViewerDisplayVte *self G_GNUC_UNUSED,
                                  gpointer data G_GNUC_UNUSED, int size G_GNUC_UNUSED)
{
}
void virt_viewer_display_vte_zoom_in(VirtViewerDisplayVte *self G_GNUC_UNUSED)
{
}
void virt_viewer_display_vte_zoom_out(VirtViewerDisplayVte *self G_GNUC_UNUSED)
{
}
void virt_viewer_display_vte_zoom_reset(VirtViewerDisplayVte *self G_GNUC_UNUSED)
{
}
#endif
