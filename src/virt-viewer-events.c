/*
 * events.c: event loop integration
 *
 * Copyright (C) 2008-2012 Daniel P. Berrange
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#ifdef G_OS_WIN32
#include <io.h>
#endif
#include <libvirt/libvirt.h>

#include "virt-viewer-events.h"
#include "virt-glib-compat.h"

static GMutex *eventlock = NULL;

struct virt_viewer_events_handle
{
    int watch;
    int fd;
    int events;
    int removed;
    GIOChannel *channel;
    guint source;
    virEventHandleCallback cb;
    void *opaque;
    virFreeCallback ff;
};

static int nextwatch = 1;
static GPtrArray *handles;

static gboolean
virt_viewer_events_dispatch_handle(GIOChannel *source G_GNUC_UNUSED,
                                   GIOCondition condition,
                                   gpointer opaque)
{
    struct virt_viewer_events_handle *data = opaque;
    int events = 0;

    if (condition & G_IO_IN)
        events |= VIR_EVENT_HANDLE_READABLE;
    if (condition & G_IO_OUT)
        events |= VIR_EVENT_HANDLE_WRITABLE;
    if (condition & G_IO_HUP)
        events |= VIR_EVENT_HANDLE_HANGUP;
    if (condition & G_IO_ERR)
        events |= VIR_EVENT_HANDLE_ERROR;

    g_debug("Dispatch handler %d %d %p", data->fd, events, data->opaque);

    (data->cb)(data->watch, data->fd, events, data->opaque);

    return TRUE;
}


static
int virt_viewer_events_add_handle(int fd,
                                  int events,
                                  virEventHandleCallback cb,
                                  void *opaque,
                                  virFreeCallback ff)
{
    struct virt_viewer_events_handle *data;
    GIOCondition cond = 0;
    int ret;

    g_mutex_lock(eventlock);

    data = g_new0(struct virt_viewer_events_handle, 1);

    if (events & VIR_EVENT_HANDLE_READABLE)
        cond |= G_IO_IN;
    if (events & VIR_EVENT_HANDLE_WRITABLE)
        cond |= G_IO_OUT;

    data->watch = nextwatch++;
    data->fd = fd;
    data->events = events;
    data->cb = cb;
    data->opaque = opaque;
#ifdef G_OS_WIN32
    g_debug("Converted fd %d to handle %"PRIiPTR, fd, _get_osfhandle(fd));
    data->channel = g_io_channel_win32_new_socket(_get_osfhandle(fd));
#else
    data->channel = g_io_channel_unix_new(fd);
#endif
    data->ff = ff;

    g_debug("Add handle %d %d %p", data->fd, events, data->opaque);

    if (events != 0) {
        data->source = g_io_add_watch(data->channel,
                                      cond,
                                      virt_viewer_events_dispatch_handle,
                                      data);
    }

    g_ptr_array_add(handles, data);

    ret = data->watch;

    g_mutex_unlock(eventlock);

    return ret;
}

static struct virt_viewer_events_handle *
virt_viewer_events_find_handle(int watch)
{
    guint i;

    for (i = 0 ; i < handles->len ; i++) {
        struct virt_viewer_events_handle *h = g_ptr_array_index(handles, i);

        if (h == NULL) {
            g_warn_if_reached ();
            continue;
        }

        if ((h->watch == watch) && !h->removed) {
            return h;
        }
    }

    return NULL;
}

static void
virt_viewer_events_update_handle(int watch,
                                 int events)
{
    struct virt_viewer_events_handle *data;

    g_mutex_lock(eventlock);

    data = virt_viewer_events_find_handle(watch);

    if (!data) {
        g_debug("Update for missing handle watch %d", watch);
        goto cleanup;
    }

    if (events) {
        GIOCondition cond = 0;
        if (events == data->events)
            goto cleanup;

        if (data->source)
            g_source_remove(data->source);

        cond |= G_IO_HUP;
        if (events & VIR_EVENT_HANDLE_READABLE)
            cond |= G_IO_IN;
        if (events & VIR_EVENT_HANDLE_WRITABLE)
            cond |= G_IO_OUT;
        data->source = g_io_add_watch(data->channel,
                                      cond,
                                      virt_viewer_events_dispatch_handle,
                                      data);
        data->events = events;
    } else {
        if (!data->source)
            goto cleanup;

        g_source_remove(data->source);
        data->source = 0;
        data->events = 0;
    }

cleanup:
    g_mutex_unlock(eventlock);
}


static gboolean
virt_viewer_events_cleanup_handle(gpointer user_data)
{
    struct virt_viewer_events_handle *data = user_data;

    g_debug("Cleanup of handle %p", data);
    g_return_val_if_fail(data != NULL, FALSE);

    if (data->ff)
        (data->ff)(data->opaque);

    g_mutex_lock(eventlock);
    g_ptr_array_remove_fast(handles, data);
    g_mutex_unlock(eventlock);

    return FALSE;
}


static int
virt_viewer_events_remove_handle(int watch)
{
    struct virt_viewer_events_handle *data;
    int ret = -1;

    g_mutex_lock(eventlock);

    data = virt_viewer_events_find_handle(watch);

    if (!data) {
        g_debug("Remove of missing watch %d", watch);
        goto cleanup;
    }

    g_debug("Remove handle %d %d", watch, data->fd);

    if (data->source != 0) {
        g_source_remove(data->source);
        data->source = 0;
        data->events = 0;
    }

    g_warn_if_fail(data->channel != NULL);
    g_io_channel_unref(data->channel);
    data->channel = NULL;

    /* since the actual watch deletion is done asynchronously, a update_handle call may
     * reschedule the watch before it's fully deleted, that's why we need to mark it as
     * 'removed' to prevent reuse
     */
    data->removed = TRUE;
    g_idle_add(virt_viewer_events_cleanup_handle, data);
    ret = 0;

cleanup:
    g_mutex_unlock(eventlock);
    return ret;
}

struct virt_viewer_events_timeout
{
    int timer;
    int interval;
    int removed;
    guint source;
    virEventTimeoutCallback cb;
    void *opaque;
    virFreeCallback ff;
};


static int nexttimer = 1;
static GPtrArray *timeouts;

static gboolean
virt_viewer_events_dispatch_timeout(void *opaque)
{
    struct virt_viewer_events_timeout *data = opaque;
    g_debug("Dispatch timeout %p %p %d %p", data, data->cb, data->timer, data->opaque);
    (data->cb)(data->timer, data->opaque);

    return TRUE;
}

static int
virt_viewer_events_add_timeout(int interval,
                               virEventTimeoutCallback cb,
                               void *opaque,
                               virFreeCallback ff)
{
    struct virt_viewer_events_timeout *data;
    int ret;

    g_mutex_lock(eventlock);

    data = g_new0(struct virt_viewer_events_timeout, 1);

    data->timer = nexttimer++;
    data->interval = interval;
    data->cb = cb;
    data->opaque = opaque;
    data->ff = ff;
    if (interval >= 0)
        data->source = g_timeout_add(interval,
                                     virt_viewer_events_dispatch_timeout,
                                     data);

    g_ptr_array_add(timeouts, data);

    g_debug("Add timeout %p %d %p %p %d", data, interval, cb, opaque, data->timer);

    ret = data->timer;

    g_mutex_unlock(eventlock);

    return ret;
}


static struct virt_viewer_events_timeout *
virt_viewer_events_find_timeout(int timer)
{
    guint i;

    g_return_val_if_fail(timeouts != NULL, NULL);

    for (i = 0 ; i < timeouts->len ; i++) {
        struct virt_viewer_events_timeout *t = g_ptr_array_index(timeouts, i);

        if (t == NULL) {
            g_warn_if_reached ();
            continue;
        }

        if ((t->timer == timer) && !t->removed) {
            return t;
        }
    }

    return NULL;
}


static void
virt_viewer_events_update_timeout(int timer,
                                  int interval)
{
    struct virt_viewer_events_timeout *data;

    g_mutex_lock(eventlock);

    data = virt_viewer_events_find_timeout(timer);
    if (!data) {
        g_debug("Update of missing timer %d", timer);
        goto cleanup;
    }

    g_debug("Update timeout %p %d %d", data, timer, interval);

    if (interval >= 0) {
        if (data->source)
            g_source_remove(data->source);

        data->interval = interval;
        data->source = g_timeout_add(data->interval,
                                     virt_viewer_events_dispatch_timeout,
                                     data);
    } else {
        if (!data->source)
            goto cleanup;

        g_source_remove(data->source);
        data->source = 0;
    }

cleanup:
    g_mutex_unlock(eventlock);
}


static gboolean
virt_viewer_events_cleanup_timeout(gpointer user_data)
{
    struct virt_viewer_events_timeout *data = user_data;

    g_debug("Cleanup of timeout %p", data);
    g_return_val_if_fail(data != NULL, FALSE);

    if (data->ff)
        (data->ff)(data->opaque);

    g_mutex_lock(eventlock);
    g_ptr_array_remove_fast(timeouts, data);
    g_mutex_unlock(eventlock);

    return FALSE;
}


static int
virt_viewer_events_remove_timeout(int timer)
{
    struct virt_viewer_events_timeout *data;
    int ret = -1;

    g_mutex_lock(eventlock);

    data = virt_viewer_events_find_timeout(timer);
    if (!data) {
        g_debug("Remove of missing timer %d", timer);
        goto cleanup;
    }

    g_debug("Remove timeout %p %d", data, timer);

    if (data->source != 0) {
        g_source_remove(data->source);
        data->source = 0;
    }

    /* since the actual timeout deletion is done asynchronously, a update_timeout call may
     * reschedule the timeout before it's fully deleted, that's why we need to mark it as
     * 'removed' to prevent reuse
     */
    data->removed = TRUE;
    g_idle_add(virt_viewer_events_cleanup_timeout, data);
    ret = 0;

cleanup:
    g_mutex_unlock(eventlock);
    return ret;
}

static gpointer event_register_once(gpointer data G_GNUC_UNUSED)
{
    eventlock = g_mutex_new();
    timeouts = g_ptr_array_new_with_free_func(g_free);
    handles = g_ptr_array_new_with_free_func(g_free);
    virEventRegisterImpl(virt_viewer_events_add_handle,
                         virt_viewer_events_update_handle,
                         virt_viewer_events_remove_handle,
                         virt_viewer_events_add_timeout,
                         virt_viewer_events_update_timeout,
                         virt_viewer_events_remove_timeout);

    return NULL;
}

void virt_viewer_events_register(void) {
    static GOnce once = G_ONCE_INIT;

    g_once(&once, event_register_once, NULL);
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 * End:
 */
