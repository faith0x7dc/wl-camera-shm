/*
 * The MIT License (MIT)
 *
 * Copyright © 2014 faith
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * refer: weston/simple-shm
 *   http://cgit.freedesktop.org/wayland/weston/
 */
#ifndef _WAYLAND_H
#define _WAYLAND_H

/*======================================
	Header include
======================================*/

#include <stdbool.h>


/*======================================
	Structures
======================================*/

struct wayland_ctx;


/*======================================
	Prototypes
======================================*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct wayland_ctx *wayland_init(unsigned int width, unsigned int height);
void wayland_terminate(struct wayland_ctx *ctx);
unsigned int wayland_get_width(struct wayland_ctx *ctx);
unsigned int wayland_get_height(struct wayland_ctx *ctx);
bool wayland_is_running();
int wayland_dispatch_event(struct wayland_ctx *ctx);
bool wayland_queue_buffer(struct wayland_ctx *ctx, void *buff);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _WAYLAND_H */
