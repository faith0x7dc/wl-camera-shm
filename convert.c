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

/*======================================
	Header include
======================================*/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "common.h"
#include "convert.h"


/*======================================
	Macro
======================================*/

#define ROUND(val, min, max) do {	\
	if (val < min) val = min;		\
	else if (val > max) val = max;	\
} while(0)


/*======================================
	Public function
======================================*/

bool
convert_yuyv_to_bgrx8888(void *dst, void *src, uint32_t width, uint32_t height)
{
	unsigned char *src_p = src;
	unsigned char *dst_p = dst;

	int i, j, k = 0;

	if (!src || !dst || !width || !height)
		return false;

	/*
	 * YUV to RGB
	 *
	 * R = Y +                   1.4020(V - 128)
	 * G = Y - 0.3441(U - 128) - 0.7139(V - 128)
	 * B = Y + 1.7718(U - 128) - 0.0012(V - 128)
	 *
	 * ->
	 * 256R = 256Y +                359(V - 128)
	 * 256G = 256Y -  88(U - 128) - 183(V - 128)
	 * 256B = 256Y + 454(U - 128) -   0(V - 128)
	 *
	 * ->
	 * 256R = 256Y + [                359(V - 128) ]
	 * 256G = 256Y - [  88(U - 128) + 183(V - 128) ]
	 * 256B = 256Y + [ 454(U - 128)                ]
	 *
	 * ->
	 * R = Y + [                359(V - 128) ] >> 8
	 * G = Y - [  88(U - 128) + 183(V - 128) ] >> 8
	 * B = Y + [ 454(U - 128)                ] >> 8
	 *
	 */
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			int y, u, v;
			int r, g, b;

			y = src_p[2 * k];
			u = src_p[1] - 128;
			v = src_p[3] - 128;

			r = y + (             (359 * v)  >> 8);
			g = y - ((( 88 * u) + (183 * v)) >> 8);
			b = y + ( (454 * u)              >> 8);

			ROUND(r, 0, 255);
			ROUND(g, 0, 255);
			ROUND(b, 0, 255);

			dst_p[0] = b;
			dst_p[1] = g;
			dst_p[2] = r;
			dst_p[3] = 0xff;

			dst_p += 4;

			if (k++) {
				k = 0;
				src_p += 4;
			}
		}
	}

	return true;
}

