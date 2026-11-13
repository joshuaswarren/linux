/* SPDX-License-Identifier: GPL-2.0-only OR MIT */
/*
 * Software camera indicator: pure state and policy.
 *
 * The kernel draws a small opaque square on a swap layer that userspace
 * cannot address. It is best effort: the camera never waits on it and never
 * depends on it. It is NOT the secure indicator Apple's firmware draws
 * outside the OS: root who can run its own kernel defeats it.
 */
#ifndef DCP_INDICATOR_H
#define DCP_INDICATOR_H

#include <linux/types.h>
#include <linux/errno.h>

enum dcp_ind_state {
	DCP_IND_OFF,
	/* Requested; no present carrying the square has completed yet. */
	DCP_IND_PENDING,
	/* A present carrying the square completed. */
	DCP_IND_LIT,
};

struct dcp_indicator {
	enum dcp_ind_state state;
};

/* One owner at a time: a second request cannot reset a running one. */
static inline int dcp_ind_begin(struct dcp_indicator *ind)
{
	if (ind->state != DCP_IND_OFF)
		return -EBUSY;
	ind->state = DCP_IND_PENDING;
	return 0;
}

/* A completed present: the first one that carried the square confirms it. */
static inline void dcp_ind_present_done(struct dcp_indicator *ind, bool dot_shown)
{
	if (ind->state == DCP_IND_PENDING && dot_shown)
		ind->state = DCP_IND_LIT;
}

static inline void dcp_ind_end(struct dcp_indicator *ind)
{
	ind->state = DCP_IND_OFF;
}

/*
 * The reserved plane accepts no userspace framebuffer. A disabled plane, or
 * the kernel's own framebuffer, is allowed.
 */
static inline bool dcp_ind_plane_ok(bool reserved_plane, bool fb_is_indicator,
				    bool has_fb)
{
	if (!reserved_plane)
		return true;
	return !has_fb || fb_is_indicator;
}

#endif /* DCP_INDICATOR_H */
