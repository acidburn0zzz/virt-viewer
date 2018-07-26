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
#ifndef _VIRT_VIEWER_DISPLAY_VTE_H
#define _VIRT_VIEWER_DISPLAY_VTE_H

#include <glib-object.h>

#include "virt-viewer-display.h"

G_BEGIN_DECLS

#define VIRT_VIEWER_TYPE_DISPLAY_VTE virt_viewer_display_vte_get_type()

#define VIRT_VIEWER_DISPLAY_VTE(obj)                                    \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIRT_VIEWER_TYPE_DISPLAY_VTE, VirtViewerDisplayVte))

#define VIRT_VIEWER_DISPLAY_VTE_CLASS(klass)                            \
    (G_TYPE_CHECK_CLASS_CAST ((klass), VIRT_VIEWER_TYPE_DISPLAY_VTE, VirtViewerDisplayVteClass))

#define VIRT_VIEWER_IS_DISPLAY_VTE(obj)                                 \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIRT_VIEWER_TYPE_DISPLAY_VTE))

#define VIRT_VIEWER_IS_DISPLAY_VTE_CLASS(klass)                         \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), VIRT_VIEWER_TYPE_DISPLAY_VTE))

#define VIRT_VIEWER_DISPLAY_VTE_GET_CLASS(obj)                          \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), VIRT_VIEWER_TYPE_DISPLAY_VTE, VirtViewerDisplayVteClass))

typedef struct _VirtViewerDisplayVte VirtViewerDisplayVte;
typedef struct _VirtViewerDisplayVteClass VirtViewerDisplayVteClass;
typedef struct _VirtViewerDisplayVtePrivate VirtViewerDisplayVtePrivate;

struct _VirtViewerDisplayVte {
    VirtViewerDisplay parent;

    VirtViewerDisplayVtePrivate *priv;
};

struct _VirtViewerDisplayVteClass {
    VirtViewerDisplayClass parent_class;
};

GType virt_viewer_display_vte_get_type(void);

GtkWidget* virt_viewer_display_vte_new(VirtViewerSession *session, const char *name);

void virt_viewer_display_vte_feed(VirtViewerDisplayVte *vte, gpointer data, int size);

void virt_viewer_display_vte_zoom_reset(VirtViewerDisplayVte *vte);
void virt_viewer_display_vte_zoom_in(VirtViewerDisplayVte *vte);
void virt_viewer_display_vte_zoom_out(VirtViewerDisplayVte *vte);

G_END_DECLS

#endif /* _VIRT_VIEWER_DISPLAY_VTE_H */
/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 * End:
 */
