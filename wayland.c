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

/*======================================
	Header include
======================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <wayland-client.h>

#include "common.h"
#include "wayland.h"


/*======================================
	Structures
======================================*/

struct wayland_ctx;

struct display {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_shell *shell;
	struct wl_shm *shm;
	uint32_t formats;

	struct wayland_ctx *ctx;
};

struct buffer {
	struct wl_buffer *buffer;
	void *shm_data;
	int busy;
};

struct window {
	struct display *display;
	int width, height;
	struct wl_surface *surface;
	struct wl_shell_surface *shell_surface;
	struct buffer buffers[2];
	struct buffer *prev_buffer;
	struct wl_callback *callback;
};

struct wayland_ctx {
	struct display *display;
	struct window *window;
	unsigned char *buffer;
	bool buffer_empty;
};


/*======================================
	Prototypes
======================================*/

static void signal_int(int signum);

static struct display *create_display(void);
static void destroy_display(struct display *display);

static struct window *create_window(struct display *display, int width, int height);
static void destroy_window(struct window *window);

static int create_shm_buffer(struct display *display, struct buffer *buffer, int width, int height, uint32_t format);
static int create_anonymous_file(off_t size);
static int set_cloexec_or_close(int fd);
static void buffer_release(void *data, struct wl_buffer *buffer);

static void redraw(void *data, struct wl_callback *callback, uint32_t time);
static struct buffer *window_next_buffer(struct window *window);

static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name);

static void handle_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial);
static void handle_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height);
static void handle_popup_done(void *data, struct wl_shell_surface *shell_surface);

static void shm_format(void *data, struct wl_shm *wl_shm, uint32_t format);


/*======================================
	Variables
======================================*/

static const struct wl_buffer_listener buffer_listener = {
	buffer_release
};

static const struct wl_callback_listener frame_listener = {
	redraw
};

static const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove
};

static const struct wl_shell_surface_listener shell_surface_listener = {
	handle_ping,
	handle_configure,
	handle_popup_done
};

struct wl_shm_listener shm_listener = {
	shm_format
};

static int running = 1;


/*======================================
	Public functions
======================================*/

struct wayland_ctx *
wayland_init(unsigned int width, unsigned int height)
{
	struct sigaction sigint;
	struct display *display;
	struct window *window;
	struct wayland_ctx *ctx;

	ctx = (struct wayland_ctx *)malloc(sizeof(struct wayland_ctx));
	if (!ctx)
		return NULL;

	ctx->buffer = (unsigned char *)malloc(width * height * 4);
	if (!ctx->buffer) {
		free(ctx);
		return NULL;
	}

	display = create_display();
	window = create_window(display, width, height);
	if (!window) {
		free(ctx->buffer);
		free(ctx);
		return NULL;
	}

	sigint.sa_handler = signal_int;
	sigemptyset(&sigint.sa_mask);
	sigint.sa_flags = SA_RESETHAND;
	sigaction(SIGINT, &sigint, NULL);

	/* Initialise damage to full surface, so the padding gets painted */
	wl_surface_damage(window->surface, 0, 0,
		window->width, window->height);

	redraw(window, NULL, 0);

	ctx->display = display;
	ctx->window = window;
	ctx->buffer_empty = true;

	display->ctx = ctx;

	return ctx;
}

void
wayland_terminate(struct wayland_ctx *ctx)
{
	if (!ctx)
		return;

	destroy_window(ctx->window);
	destroy_display(ctx->display);

	free(ctx->buffer);
	free(ctx);
}

unsigned int
wayland_get_width(struct wayland_ctx *ctx)
{
	if (!ctx)
		return 0;

	return ctx->window->width;
}

unsigned int
wayland_get_height(struct wayland_ctx *ctx)
{
	if (!ctx)
		return 0;

	return ctx->window->height;
}

bool
wayland_is_running()
{
	return running;
}

int
wayland_dispatch_event(struct wayland_ctx *ctx)
{
	if (!ctx)
		return -1;

	return wl_display_dispatch(ctx->display->display);
}

bool
wayland_queue_buffer(struct wayland_ctx *ctx, void *buff)
{
	if (!ctx)
		return false;

	if (!ctx->buffer_empty)
		return false;

	memcpy(ctx->buffer, buff, wayland_get_width(ctx) * wayland_get_height(ctx) * 4);
	ctx->buffer_empty = false;

	return true;
}


/*======================================
	Inner functions
======================================*/

static void
signal_int(int signum)
{
	running = 0;
}

static struct display *
create_display(void)
{
	struct display *display;

	display = malloc(sizeof *display);
	if (display == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	display->display = wl_display_connect(NULL);
	assert(display->display);

	display->formats = 0;
	display->registry = wl_display_get_registry(display->display);
	wl_registry_add_listener(display->registry,
		&registry_listener, display);
	wl_display_roundtrip(display->display);
	if (display->shm == NULL) {
		fprintf(stderr, "No wl_shm global\n");
		exit(1);
	}

	wl_display_roundtrip(display->display);

	if (!(display->formats & (1 << WL_SHM_FORMAT_XRGB8888))) {
		fprintf(stderr, "WL_SHM_FORMAT_XRGB32 not available\n");
		exit(1);
	}

	wl_display_get_fd(display->display);

	return display;
}

static void
destroy_display(struct display *display)
{
	if (display->shm)
		wl_shm_destroy(display->shm);

	if (display->shell)
		wl_shell_destroy(display->shell);

	if (display->compositor)
		wl_compositor_destroy(display->compositor);

	wl_registry_destroy(display->registry);
	wl_display_flush(display->display);
	wl_display_disconnect(display->display);
	free(display);
}

static struct window *
create_window(struct display *display, int width, int height)
{
	struct window *window;

	window = calloc(1, sizeof *window);
	if (!window)
		return NULL;

	window->callback = NULL;
	window->display = display;
	window->width = width;
	window->height = height;
	window->surface = wl_compositor_create_surface(display->compositor);
	window->shell_surface = wl_shell_get_shell_surface(display->shell, window->surface);

	if (window->shell_surface)
		wl_shell_surface_add_listener(window->shell_surface, &shell_surface_listener, window);

	wl_shell_surface_set_title(window->shell_surface, "simple-shm");

	wl_shell_surface_set_toplevel(window->shell_surface);

	return window;
}

static void
destroy_window(struct window *window)
{
	if (window->callback)
		wl_callback_destroy(window->callback);

	if (window->buffers[0].buffer)
		wl_buffer_destroy(window->buffers[0].buffer);
	if (window->buffers[1].buffer)
		wl_buffer_destroy(window->buffers[1].buffer);

	wl_shell_surface_destroy(window->shell_surface);
	wl_surface_destroy(window->surface);
	free(window);
}

static int
create_shm_buffer(struct display *display, struct buffer *buffer,
	int width, int height, uint32_t format)
{
	struct wl_shm_pool *pool;
	int fd, size, stride;
	void *data;

	stride = width * 4;
	size = stride * height;

	fd = create_anonymous_file(size);
	if (fd < 0) {
		fprintf(stderr, "creating a buffer file for %d B failed: %m\n", size);
		return -1;
	}

	data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		fprintf(stderr, "mmap failed: %m\n");
		close(fd);
		return -1;
	}

	pool = wl_shm_create_pool(display->shm, fd, size);
	buffer->buffer = wl_shm_pool_create_buffer(pool, 0,
						width, height,
						stride, format);
	wl_buffer_add_listener(buffer->buffer, &buffer_listener, buffer);
	wl_shm_pool_destroy(pool);
	close(fd);

	buffer->shm_data = data;

	return 0;
}

static int
create_anonymous_file(off_t size)
{
	static const char template[] = "/weston-shared-XXXXXX";
	const char *path;
	char *name;
	int fd;
	int ret;

	path = getenv("XDG_RUNTIME_DIR");
	if (!path) {
		errno = ENOENT;
		return -1;
	}

	name = malloc(strlen(path) + sizeof(template));
	if (!name)
		return -1;

	strcpy(name, path);
	strcat(name, template);

	fd = mkstemp(name);
	if (fd >= 0) {
		fd = set_cloexec_or_close(fd);
		unlink(name);
	}

	free(name);

	if (fd < 0)
		return -1;

	ret = ftruncate(fd, size);
	if (ret < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

static int
set_cloexec_or_close(int fd)
{
	long flags;

	if (fd == -1)
		return -1;

	flags = fcntl(fd, F_GETFD);
	if (flags == -1)
		goto err;

	if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
		goto err;

	return fd;

err:
	close(fd);
	return -1;
}

static void
buffer_release(void *data, struct wl_buffer *buffer)
{
	struct buffer *mybuf = data;

	mybuf->busy = 0;
}

static void
redraw(void *data, struct wl_callback *callback, uint32_t time)
{
	struct window *window = data;
	struct buffer *buffer;
	struct wayland_ctx *ctx = window->display->ctx;

	if (callback && ctx && ctx->buffer_empty)
		return;

	buffer = window_next_buffer(window);
	if (!buffer) {
		fprintf(stderr,
			!callback ? "Failed to create the first buffer.\n" :
			"Both buffers busy at redraw(). Server bug?\n");
		abort();
	}

	if (ctx && !ctx->buffer_empty) {
		memcpy(buffer->shm_data, ctx->buffer, window->width * window->height * 4);
		ctx->buffer_empty = true;
	}

	wl_surface_attach(window->surface, buffer->buffer, 0, 0);
	wl_surface_damage(window->surface, 0, 0, window->width, window->height);

	if (callback)
		wl_callback_destroy(callback);

	window->callback = wl_surface_frame(window->surface);
	wl_callback_add_listener(window->callback, &frame_listener, window);
	wl_surface_commit(window->surface);
	buffer->busy = 1;
}

static struct buffer *
window_next_buffer(struct window *window)
{
	struct buffer *buffer;
	int ret = 0;

	if (!window->buffers[0].busy)
		buffer = &window->buffers[0];
	else if (!window->buffers[1].busy)
		buffer = &window->buffers[1];
	else
		return NULL;

	if (!buffer->buffer) {
		ret = create_shm_buffer(window->display, buffer,
				window->width, window->height,
				WL_SHM_FORMAT_XRGB8888);

		if (ret < 0)
			return NULL;

		memset(buffer->shm_data, 0xff, window->width * window->height * 4);
	}

	return buffer;
}

static void
registry_handle_global(void *data, struct wl_registry *registry,
	uint32_t id, const char *interface, uint32_t version)
{
	struct display *d = data;

	if (strcmp(interface, "wl_compositor") == 0) {
		d->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
	} else if (strcmp(interface, "wl_shell") == 0) {
		d->shell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
	} else if (strcmp(interface, "wl_shm") == 0) {
		d->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
		wl_shm_add_listener(d->shm, &shm_listener, d);
	}
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
	uint32_t name)
{
}

static void
handle_ping(void *data, struct wl_shell_surface *shell_surface,
	uint32_t serial)
{
	wl_shell_surface_pong(shell_surface, serial);
}

static void
handle_configure(void *data, struct wl_shell_surface *shell_surface,
	uint32_t edges, int32_t width, int32_t height)
{
}

static void
handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static void
shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
	struct display *d = data;

	d->formats |= (1 << format);
}

