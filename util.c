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

#include <stdio.h>
#include <sys/time.h>

#include "common.h"
#include "util.h"


/*======================================
	Constant
======================================*/

#define FPS_INTERVAL	5000	/* calc fps per 5000 [ms] */

/*======================================
	Variable
======================================*/

static int frame_count = -1;
static struct timeval old_time, new_time;


/*======================================
	Public function
======================================*/

void
util_show_fps(void)
{
	unsigned int ms;

	if (frame_count < 0) {
		gettimeofday(&old_time, NULL);
		frame_count = 0;
	}

	frame_count++;

	gettimeofday(&new_time, NULL);
	ms = (new_time.tv_sec * 1e3 + new_time.tv_usec * 1e-3)
	   - (old_time.tv_sec * 1e3 + old_time.tv_usec * 1e-3);
	if (ms >= FPS_INTERVAL) {
		printf("%d frames in %d sec. fps = %f\n", frame_count, (int)(FPS_INTERVAL * 1e-3), frame_count / (FPS_INTERVAL * 1e-3));
		frame_count = 0;
		old_time = new_time;
	}
}

