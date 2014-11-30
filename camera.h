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
 */
#ifndef _CAMERA_H
#define _CAMERA_H

/*======================================
	Header include
======================================*/

#include <stdint.h>

/*======================================
	Structure
======================================*/

struct camera_ctx;

/*======================================
	Prototype
======================================*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct camera_ctx *camera_init(char *dev_name);
void camera_terminate(struct camera_ctx *ctx);

bool camera_start_capturing(struct camera_ctx *ctx);
bool camera_stop_capturing(struct camera_ctx *ctx);

bool camera_read_frame(struct camera_ctx *ctx, void *dest, unsigned int dest_size);

uint32_t camera_get_width(struct camera_ctx *ctx);
uint32_t camera_get_height(struct camera_ctx *ctx);
uint32_t camera_get_frame_size(struct camera_ctx *ctx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _CAMERA_H */
