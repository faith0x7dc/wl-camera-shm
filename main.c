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
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <getopt.h>

#include "common.h"
#include "camera.h"
#include "wayland.h"
#include "convert.h"
#include "util.h"


/*======================================
	Constant
======================================*/

#define DEFAULT_DEVICE_NAME		"/dev/video0"


/*======================================
	Prototype
======================================*/

static void usage(FILE *fp, int argc, char *argv[]);


/*======================================
	Public function
======================================*/

int
main(int argc, char *argv[])
{
	static const char short_options[] = "d:qh";
	static const struct option long_options[] = {
		{ "device",	required_argument,	NULL, 'd' },
		{ "quiet",  no_argument,		NULL, 'q' },
		{ "help",	no_argument,		NULL, 'h' },
		{ 0, 0, 0, 0 }
	};

	struct camera_ctx *camera_ctx;
	struct wayland_ctx *wayland_ctx;
	char *dev_name;
	unsigned char *buff, *converted;
	bool ret;
	bool quiet = false;

	dev_name = DEFAULT_DEVICE_NAME;

	do {
		int idx;
		int c;

		c = getopt_long(argc, argv,
				short_options, long_options, &idx);

		if (-1 == c)
			break;

		switch (c) {
		case 0: /* getopt_long() flag */
			break;

		case 'd':
			dev_name = optarg;
			break;

		case 'q':
			quiet = true;
			break;

		case 'h':
			usage(stdout, argc, argv);
			exit(EXIT_SUCCESS);

		default:
			usage(stderr, argc, argv);
			exit(EXIT_FAILURE);
		}
	} while (1);

	camera_ctx = camera_init(dev_name);
	if (!camera_ctx) {
		exit(EXIT_FAILURE);
	}

	if (!camera_start_capturing(camera_ctx)) {
		camera_stop_capturing(camera_ctx);
		camera_terminate(camera_ctx);
		exit(EXIT_FAILURE);
	}

	buff = (unsigned char *)malloc(camera_get_frame_size(camera_ctx));
	if (!buff) {
		LOG_ERROR("Out of Memory");

		camera_stop_capturing(camera_ctx);
		camera_terminate(camera_ctx);
		exit(EXIT_FAILURE);
	}

	ret = camera_read_frame(camera_ctx, buff, camera_get_frame_size(camera_ctx));
	if (!ret) {
		free(buff);
		camera_stop_capturing(camera_ctx);
		camera_terminate(camera_ctx);
		exit(EXIT_FAILURE);
	}

	converted = (unsigned char *)malloc(camera_get_width(camera_ctx) * camera_get_height(camera_ctx) * 4);
	if (!converted) {
		LOG_ERROR("Out of Memory");

		free(buff);
		camera_stop_capturing(camera_ctx);
		camera_terminate(camera_ctx);
		exit(EXIT_FAILURE);
	}

	ret = convert_yuyv_to_bgrx8888(converted, buff, camera_get_width(camera_ctx), camera_get_height(camera_ctx));
	if (!ret) {
		free(converted);
		free(buff);
		camera_stop_capturing(camera_ctx);
		camera_terminate(camera_ctx);
		exit(EXIT_FAILURE);
	}

	wayland_ctx = wayland_init(camera_get_width(camera_ctx), camera_get_height(camera_ctx));
	if (!wayland_ctx) {
		free(converted);
		free(buff);
		camera_stop_capturing(camera_ctx);
		camera_terminate(camera_ctx);
		exit(EXIT_FAILURE);
	}

	while (wayland_is_running()) {
		if (!quiet)
			util_show_fps();

		if (wayland_queue_buffer(wayland_ctx, converted)) {

			ret = camera_read_frame(camera_ctx, buff, camera_get_frame_size(camera_ctx));
			if (!ret)
				break;

			ret =  convert_yuyv_to_bgrx8888(converted, buff, camera_get_width(camera_ctx), camera_get_height(camera_ctx));
			if (!ret)
				break;
		}

		if (wayland_dispatch_event(wayland_ctx) < 0)
			break;
	}

	wayland_terminate(wayland_ctx);

	free(converted);
	free(buff);

	camera_stop_capturing(camera_ctx);
	camera_terminate(camera_ctx);

	return 0;
}

/*======================================
	Inner function
======================================*/

static void
usage(FILE *fp, int argc, char *argv[])
{
	fprintf(fp,
		 "Usage: %s [options]\n\n"
		 "Version 0.1\n"
		 "Options:\n"
		 "-d | --device name   Video device name [%s]\n"
		 "-q | --quiet         Quiet mode\n"
		 "-h | --help	       Print this message\n"
		 "",
		 argv[0], DEFAULT_DEVICE_NAME);
}

