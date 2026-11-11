// SPDX-License-Identifier: GPL-2.0-only OR MIT
/*
 * Apple RTKit IPC library
 * Copyright (C) The Asahi Linux Contributors
 */
#include "rtkit-internal.h"

#define FOURCC(a, b, c, d) \
	(((u32)(a) << 24) | ((u32)(b) << 16) | ((u32)(c) << 8) | ((u32)(d)))

#define APPLE_RTKIT_CRASHLOG_STR FOURCC('C', 's', 't', 'r')
#define APPLE_RTKIT_CRASHLOG_VERSION FOURCC('C', 'v', 'e', 'r')
#define APPLE_RTKIT_CRASHLOG_MBOX FOURCC('C', 'm', 'b', 'x')
#define APPLE_RTKIT_CRASHLOG_TIME FOURCC('C', 't', 'i', 'm')
#define APPLE_RTKIT_CRASHLOG_REGS FOURCC('C', 'r', 'g', '8')
#define APPLE_RTKIT_CRASHLOG_CORE_STATE FOURCC('C', 's', 't', 'a')
#define APPLE_RTKIT_CRASHLOG_STACK FOURCC('C', 'c', 's', 't')
#define APPLE_RTKIT_CRASHLOG_RTKIT FOURCC('C', 'r', 't', 'k')
#define APPLE_RTKIT_CRASHLOG_CCDP FOURCC('C', 'c', 'd', 'p')
#define APPLE_RTKIT_CRASHLOG_CPCT FOURCC('C', 'p', 'c', 't')

/*
 * Saved core context of the interrupted thread (PMP firmware, t6050).
 * Offsets follow the firmware exception dispatcher context layout:
 * pc at +0x100, far at +0x328, esr (u32) at +0x338. The esr is checked
 * against the esr the Crg8 section dumped; on mismatch only the values
 * print with a layout-unverified tag.
 */
#define CSTA_PC_OFF 0x100
#define CSTA_FAR_OFF 0x328
#define CSTA_ESR_OFF 0x338


/* For COMPILE_TEST on non-ARM64 architectures */
#ifndef PSR_MODE_EL0t
#define PSR_MODE_EL0t	0x00000000
#define PSR_MODE_EL1t	0x00000004
#define PSR_MODE_EL1h	0x00000005
#define PSR_MODE_EL2t	0x00000008
#define PSR_MODE_EL2h	0x00000009
#define PSR_MODE_MASK	0x0000000f
#endif

struct apple_rtkit_crashlog_mbox_entry {
	u64 msg0;
	u64 msg1;
	u32 timestamp;
	u8 _unk[4];
};
static_assert(sizeof(struct apple_rtkit_crashlog_mbox_entry) == 0x18);

struct apple_rtkit_crashlog_regs {
	u32 unk_0;
	u32 unk_4;
	u64 regs[31];
	u64 sp;
	u64 pc;
	u64 psr;
	u64 cpacr;
	u64 fpsr;
	u64 fpcr;
	u64 unk[64];
	u64 far;
	u64 unk_X;
	u64 esr;
	u64 unk_Z;
} __packed;
static_assert(sizeof(struct apple_rtkit_crashlog_regs) == 0x350);

static void apple_rtkit_crashlog_dump_str(struct apple_rtkit *rtk, u8 *bfr,
					  size_t size)
{
	u32 idx;
	u8 *ptr, *end;

	memcpy(&idx, bfr, 4);

	ptr = bfr + 4;
	end = bfr + size;
	while (ptr < end) {
		u8 *newline = memchr(ptr, '\n', end - ptr);

		if (newline) {
			u8 tmp = *newline;
			*newline = '\0';
			dev_warn(rtk->dev, "RTKit: Message (id=%x): %s\n", idx,
				 ptr);
			*newline = tmp;
			ptr = newline + 1;
		} else {
			dev_warn(rtk->dev, "RTKit: Message (id=%x): %s", idx,
				 ptr);
			break;
		}
	}
}

static void apple_rtkit_crashlog_dump_version(struct apple_rtkit *rtk, u8 *bfr,
					      size_t size)
{
	dev_warn(rtk->dev, "RTKit: Version: %s", bfr + 16);
}

static void apple_rtkit_crashlog_dump_time(struct apple_rtkit *rtk, u8 *bfr,
					   size_t size)
{
	u64 crash_time;

	memcpy(&crash_time, bfr, 8);
	dev_warn(rtk->dev, "RTKit: Crash time: %lld", crash_time);
}

static void apple_rtkit_crashlog_dump_mailbox(struct apple_rtkit *rtk, u8 *bfr,
					      size_t size)
{
	u32 type, index, i;
	size_t n_messages;
	struct apple_rtkit_crashlog_mbox_entry entry;

	if (size < 28) {
		dev_warn(rtk->dev, "RTKit: Mailbox section too small: 0x%zx", size);
		return;
	}

	memcpy(&type, bfr + 16, 4);
	memcpy(&index, bfr + 24, 4);
	n_messages = (size - 28) / sizeof(entry);

	dev_warn(rtk->dev, "RTKit: Mailbox history (type = %d, index = %d)",
		 type, index);
	for (i = 0; i < n_messages; ++i) {
		memcpy(&entry, bfr + 28 + i * sizeof(entry), sizeof(entry));
		dev_warn(rtk->dev, "RTKit:  #%03d@%08x: %016llx %016llx", i,
			 entry.timestamp, entry.msg0, entry.msg1);
	}
}

static void apple_rtkit_crashlog_dump_regs(struct apple_rtkit *rtk, u8 *bfr,
					   size_t size)
{
	struct apple_rtkit_crashlog_regs *regs;
	const char *el;
	int i;

	if (size < sizeof(*regs)) {
		dev_warn(rtk->dev, "RTKit: Regs section too small: 0x%zx", size);
		return;
	}

	regs = (struct apple_rtkit_crashlog_regs *)bfr;
	rtk->last_dumped_esr = regs->esr;

	switch (regs->psr & PSR_MODE_MASK) {
	case PSR_MODE_EL0t:
		el = "EL0t";
		break;
	case PSR_MODE_EL1t:
		el = "EL1t";
		break;
	case PSR_MODE_EL1h:
		el = "EL1h";
		break;
	case PSR_MODE_EL2t:
		el = "EL2t";
		break;
	case PSR_MODE_EL2h:
		el = "EL2h";
		break;
	default:
		el = "unknown";
		break;
	}

	dev_warn(rtk->dev, "RTKit: Exception dump:");
	dev_warn(rtk->dev, "  == Exception taken from %s ==", el);
	dev_warn(rtk->dev, "  PSR    = 0x%llx", regs->psr);
	dev_warn(rtk->dev, "  PC     = 0x%llx\n", regs->pc);
	dev_warn(rtk->dev, "  ESR    = 0x%llx\n", regs->esr);
	dev_warn(rtk->dev, "  FAR    = 0x%llx\n", regs->far);
	dev_warn(rtk->dev, "  SP     = 0x%llx\n", regs->sp);
	dev_warn(rtk->dev, "\n");

	for (i = 0; i < 31; i += 4) {
		if (i < 28)
			dev_warn(rtk->dev,
					 "  x%02d-x%02d = %016llx %016llx %016llx %016llx\n",
					 i, i + 3,
					 regs->regs[i], regs->regs[i + 1],
					 regs->regs[i + 2], regs->regs[i + 3]);
		else
			dev_warn(rtk->dev,
					 "  x%02d-x%02d = %016llx %016llx %016llx\n", i, i + 3,
					 regs->regs[i], regs->regs[i + 1], regs->regs[i + 2]);
	}

	dev_warn(rtk->dev, "\n");
}

/* A crashlog can carry several KB per section; the console is slow, so bound the output. */
#define CRASHLOG_HEX_MAX_WORDS 32
#define CRASHLOG_MAX_SECTIONS 32

static void apple_rtkit_crashlog_dump_hexwords(struct apple_rtkit *rtk,
					       u8 *bfr, size_t size)
{
	size_t i, n = min_t(size_t, size / 8, CRASHLOG_HEX_MAX_WORDS);
	u64 v[4];

	for (i = 0; i < n; i += 4) {
		size_t k, cnt = min_t(size_t, n - i, 4);

		for (k = 0; k < cnt; k++)
			memcpy(&v[k], bfr + (i + k) * 8, 8);
		for (; k < 4; k++)
			v[k] = 0;
		dev_warn(rtk->dev, "RTKit:   +%03zx = %016llx %016llx %016llx %016llx",
			 i * 8, v[0], v[1], v[2], v[3]);
	}
	if (size / 8 > n)
		dev_warn(rtk->dev, "RTKit:   (%zu more words not shown)", size / 8 - n);
}

static void apple_rtkit_crashlog_dump_core_state(struct apple_rtkit *rtk,
						 u8 *bfr, size_t size)
{
	u32 version, esr = 0;
	u64 pc = 0, far = 0;

	if (size < 4) {
		dev_warn(rtk->dev, "RTKit: Csta section too small: 0x%zx",
			 size);
		return;
	}

	memcpy(&version, bfr, 4);
	if (size >= 4 + CSTA_ESR_OFF + 4) {
		memcpy(&pc, bfr + 4 + CSTA_PC_OFF, 8);
		memcpy(&far, bfr + 4 + CSTA_FAR_OFF, 8);
		memcpy(&esr, bfr + 4 + CSTA_ESR_OFF, 4);
	}

	if (esr == rtk->last_dumped_esr && esr)
		dev_warn(rtk->dev,
			 "RTKit: Interrupted context: ELR = 0x%llx FAR = 0x%llx (ESR 0x%x matches dump)",
			 pc, far, esr);
	else
		dev_warn(rtk->dev,
			 "RTKit: Interrupted context (layout unverified, ESR 0x%x != dump 0x%llx): ELR = 0x%llx FAR = 0x%llx",
			 esr, rtk->last_dumped_esr, pc, far);

	apple_rtkit_crashlog_dump_hexwords(rtk, bfr + 4, size - 4);
}

static void apple_rtkit_crashlog_dump_stack(struct apple_rtkit *rtk,
					    u8 *bfr, size_t size)
{
	u32 idx, flags;

	if (size < 8) {
		dev_warn(rtk->dev, "RTKit: Ccst section too small: 0x%zx",
			 size);
		return;
	}

	memcpy(&idx, bfr, 4);
	memcpy(&flags, bfr + 4, 4);
	dev_warn(rtk->dev, "RTKit: Crashed stack (thread=%u flags=%x):",
		 idx, flags);
	apple_rtkit_crashlog_dump_hexwords(rtk, bfr + 8, size - 8);
}

void apple_rtkit_crashlog_dump(struct apple_rtkit *rtk, u8 *bfr, size_t size)
{
	size_t offset;
	u32 section_fourcc, section_size, payload;
	struct apple_rtkit_crashlog_header header;
	unsigned int n_sections = 0;

	memcpy(&header, bfr, sizeof(header));
	if (header.fourcc != APPLE_RTKIT_CRASHLOG_HEADER_FOURCC) {
		dev_warn(rtk->dev, "RTKit: Expected crashlog header but got %x",
			 header.fourcc);
		return;
	}

	if (header.size > size) {
		dev_warn(rtk->dev, "RTKit: Crashlog size (%x) is too large",
			 header.size);
		return;
	}

	size = header.size;
	offset = sizeof(header);

	while (offset < size) {
		if (size - offset < 16) {
			dev_warn(rtk->dev, "RTKit: Truncated crashlog section header at 0x%zx",
				 offset);
			return;
		}
		memcpy(&section_fourcc, bfr + offset, 4);
		memcpy(&section_size, bfr + offset + 12, 4);
		/* The size covers the 16 byte section header; handlers get the payload. */
		if (section_size < 16 || section_size > size - offset) {
			dev_warn(rtk->dev,
				 "RTKit: Bad crashlog section %x size 0x%x at 0x%zx",
				 section_fourcc, section_size, offset);
			return;
		}
		payload = section_size - 16;
		if (++n_sections > CRASHLOG_MAX_SECTIONS) {
			dev_warn(rtk->dev, "RTKit: Crashlog has more than %d sections, stopping",
				 CRASHLOG_MAX_SECTIONS);
			return;
		}

		switch (section_fourcc) {
		case APPLE_RTKIT_CRASHLOG_HEADER_FOURCC:
			dev_dbg(rtk->dev, "RTKit: End of crashlog reached");
			return;
		case APPLE_RTKIT_CRASHLOG_STR:
			apple_rtkit_crashlog_dump_str(rtk, bfr + offset + 16,
						      payload);
			break;
		case APPLE_RTKIT_CRASHLOG_VERSION:
			apple_rtkit_crashlog_dump_version(
				rtk, bfr + offset + 16, payload);
			break;
		case APPLE_RTKIT_CRASHLOG_MBOX:
			apple_rtkit_crashlog_dump_mailbox(
				rtk, bfr + offset + 16, payload);
			break;
		case APPLE_RTKIT_CRASHLOG_TIME:
			apple_rtkit_crashlog_dump_time(rtk, bfr + offset + 16,
						       payload);
			break;
		case APPLE_RTKIT_CRASHLOG_REGS:
			apple_rtkit_crashlog_dump_regs(rtk, bfr + offset + 16,
						       payload);
			break;
		case APPLE_RTKIT_CRASHLOG_CORE_STATE:
			apple_rtkit_crashlog_dump_core_state(
				rtk, bfr + offset + 16, payload);
			break;
		case APPLE_RTKIT_CRASHLOG_STACK:
			apple_rtkit_crashlog_dump_stack(rtk, bfr + offset + 16,
							payload);
			break;
		case APPLE_RTKIT_CRASHLOG_RTKIT:
		case APPLE_RTKIT_CRASHLOG_CCDP:
		case APPLE_RTKIT_CRASHLOG_CPCT:
			dev_warn(rtk->dev, "RTKit: State section (size=0x%x):",
				 payload);
			apple_rtkit_crashlog_dump_hexwords(rtk, bfr + offset + 16,
							   payload);
			break;
		default:
			dev_warn(rtk->dev,
				 "RTKit: Unknown crashlog section: %x",
				 section_fourcc);
		}

		offset += section_size;
	}

	dev_warn(rtk->dev,
		 "RTKit: End of crashlog reached but no footer present");
}
