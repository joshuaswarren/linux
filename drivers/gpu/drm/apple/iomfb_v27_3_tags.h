/* SPDX-License-Identifier: GPL-2.0-only OR MIT */
/*
 * IOMFB method tags of the 27.3 DCP firmware (macOS 27.3, mBoot-20457.60.65).
 *
 * 27.3 renumbered the AP-to-DCP methods of the A4xx block: 27.0's A401..A467
 * moved by +50 and A475.. by +51. Tags below A401 are unchanged, except
 * vi_set_temperature_hint (+1). Every row of DCP_V27_3_TAGS pairs the tag the
 * 27.0 tables use with the 27.3 tag.
 *
 * Source: the 27.3 kernelcache, IOMobileFramebufferAP and
 * AppleMobileDispT605X-DCP. Each method's tag is the 32-bit ASCII immediate
 * loaded before its call; M rows were matched by the method name in the kext
 * symbols; I rows were placed by the offset rule and by the in/out sizes of
 * the call (the kernelcache names the call site's function, not the method).
 */
#ifndef _APPLE_IOMFB_V27_3_TAGS_H
#define _APPLE_IOMFB_V27_3_TAGS_H

/* X(27.0 tag, 27.3 tag, basis) */
#define DCP_V27_3_TAGS(X)                                                    \
	X("A389", "A390", I) /* set_create_dfb: slot before A391 */         \
	X("A390", "A391", M) /* vi_set_temperature_hint */                   \
	X("A401", "A451", I) /* start_signal: in 0 out 4 */                  \
	X("A406", "A456", M) /* swap_start_dcp */                            \
	X("A408", "A458", I) /* swap_submit_dcp_main; struct size unchecked */ \
	X("A411", "A461", M) /* set_display_device */                        \
	X("A412", "A462", M) /* is_main_display */                           \
	X("A413", "A463", M) /* set_digital_out_mode */                      \
	X("A414", "A464", I) /* in 8 out 8, as sent */                       \
	X("A415", "A465", I) /* in 12 out 12, as sent */                     \
	X("A423", "A473", M) /* set_matrix */                                \
	X("A427", "A477", M) /* get_color_remap_mode */                      \
	X("A428", "A478", M) /* setBrightnessCorrection */                   \
	X("A450", "A500", M) /* set_parameter_dcp */                         \
	X("A453", "A503", I) /* get_dfb_state: in 4 out 8 */                 \
	X("A454", "A504", M) /* do_create_default_frame_buffer */            \
	X("A458", "A508", M) /* enable_disable_video_power_savings */        \
	X("A466", "A516", I) /* first_client_open: next to A517 */           \
	X("A467", "A517", M) /* last_client_close_dcp */                     \
	X("A475", "A526", M) /* setDisplayRefreshProperties */              \
	X("A478", "A529", M) /* flush_supportsPower */                      \
	X("A479", "A530", M) /* abort_swaps_dcp */                           \
	X("A483", "A534", M) /* update_dfb */                                \
	X("A484", "A535", M) /* setPowerState */                             \
	X("A485", "A536", I) /* isKeepOnScreen, by offset */                 \
	X("A487", "A538", M) /* set_dcp_clamshellstate */                    \
	X("A489", "A540", M) /* get_shared_disp_state */

/* The 27.3 tag for a 27.0 tag, or NULL when the tag did not change. */
static inline const char *dcp_v27_3_tag(const char tag[4])
{
#define DCP_V27_3_ROW(old, new, basis) \
	if (tag[0] == old[0] && tag[1] == old[1] && tag[2] == old[2] && tag[3] == old[3]) \
		return new;
	DCP_V27_3_TAGS(DCP_V27_3_ROW)
#undef DCP_V27_3_ROW
	return NULL;
}

#endif
