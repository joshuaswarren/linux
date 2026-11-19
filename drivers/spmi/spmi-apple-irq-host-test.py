#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only OR MIT
"""Compile selected unchanged Apple SPMI production code against tiny host mocks."""
import os
import shlex
import subprocess
import tempfile
from pathlib import Path


SOURCE = Path(__file__).with_name("spmi-apple-controller.c")


def block(source, marker):
    start = source.index(marker)
    brace = source.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + "\n"


def main():
    source = SOURCE.read_text()
    declarations = source[source.index("#define SPMI_STATUS_REG"):
                          source.index("static void apple_spmi_irq_ack_raw(")]
    functions = "".join(block(source, marker) for marker in (
        "static int apple_spmi_wait_rx_not_empty(",
        "static void apple_spmi_irq_ack_raw(",
        "static void apple_spmi_irq_mask_raw(",
        "static void apple_spmi_irq_unmask_raw(",
        "static int apple_spmi_irq_domain_translate(",
        "static int apple_spmi_init_irq(",
        "static int apple_spmi_resume_noirq(",
        "static int apple_spmi_probe(",
    ))
    harness = r'''#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef unsigned long irq_hw_number_t;
#define __iomem
#define U32_MAX UINT32_MAX
#define BIT(n) (1U << (n))
#define IRQ_TYPE_SENSE_MASK 0xf
#define IRQ_TYPE_NONE 0
#define IS_ERR(p) ((intptr_t)(p) < 0)
#define PTR_ERR(p) ((intptr_t)(p))
#define IORESOURCE_MEM 0
#define DECLARE_BITMAP(n, bits) unsigned long n[((bits) + 8*sizeof(unsigned long)-1)/(8*sizeof(unsigned long))]
#define guard(x) fake_guard
#define raw_spin_lock_irqsave(p,f) do { (void)(p); (f)=0; } while (0)
#define raw_spin_unlock_irqrestore(p,f) do { (void)(p); (void)(f); } while (0)
static void fake_guard(void *p) { (void)p; }
typedef int raw_spinlock_t;
struct mutex { int unused; };
struct completion { int unused; };
static void mutex_init(struct mutex *p) { p->unused=0; }
static void init_completion(struct completion *p) { p->unused=0; }
struct irq_domain; struct irq_domain_ops { int unused; };
struct device { void *drvdata, *fwnode, *of_node, *match_data; };
struct spmi_controller {
 struct device dev; void *data;
 int (*read_cmd)(struct spmi_controller *, u8, u8, u16, u8 *, size_t);
 int (*write_cmd)(struct spmi_controller *, u8, u8, u16, const u8 *, size_t);
 int (*cmd)(struct spmi_controller *, u8, u8);
};
struct platform_device { struct device dev; int irq, add_result; u32 *regs; bool second_resource; };
struct irq_domain { void *host_data; };
struct irq_domain_info { void *fwnode; unsigned int hwirq_max; const struct irq_domain_ops *ops; void *host_data; };
struct irq_fwspec { u32 *param; unsigned int param_count; };
struct apple_spmi;
struct irq_data { irq_hw_number_t hwirq; };
struct irq_desc;
static struct spmi_controller controller;
static _Alignas(max_align_t) unsigned char storage[4096];
static struct irq_domain domain;
static unsigned int domain_limit, write_count, poll_calls;
static unsigned long write_offsets[128];
static bool firmware_resume;
static u32 *active_regs;
struct apple_spmi *spmi_controller_get_drvdata(struct spmi_controller *c) { return c->data; }
static void *dev_get_drvdata(struct device *d) { return d->drvdata; }
static struct spmi_controller *devm_spmi_controller_alloc(struct device *d, size_t n)
{ (void)d; memset(&controller, 0, sizeof(controller)); memset(storage, 0, sizeof(storage)); controller.data=storage; assert(n <= sizeof(storage)); return &controller; }
static void *device_get_match_data(struct device *d) { return d->match_data; }
static void platform_set_drvdata(struct platform_device *p, void *v) { p->dev.drvdata=v; }
static u32 *devm_platform_ioremap_resource(struct platform_device *p, int i) { (void)i; return p->regs; }
static void *platform_get_resource(struct platform_device *p, int t, int i) { (void)t; return i == 1 && p->second_resource ? p : NULL; }
static int platform_get_irq_optional(struct platform_device *p, int i) { (void)i; return p->irq; }
static int devm_spmi_controller_add(struct device *d, struct spmi_controller *c) { (void)d; (void)c; return 0; }
static int dev_err_probe(struct device *d, int e, const char *f, ...) { (void)d; (void)f; return e; }
static void dev_err(struct device *d, const char *f, ...) { (void)d; (void)f; }
static unsigned long usecs_to_jiffies(unsigned long n) { return n; }
static unsigned long wait_for_completion_timeout(struct completion *c, unsigned long n) { (void)c;(void)n;return 1; }
static int stub_read(struct spmi_controller *c,u8 o,u8 s,u16 a,u8*b,size_t n) { (void)c;(void)o;(void)s;(void)a;(void)b;(void)n;return 0; }
static int stub_write(struct spmi_controller *c,u8 o,u8 s,u16 a,const u8*b,size_t n) { (void)c;(void)o;(void)s;(void)a;(void)b;(void)n;return 0; }
static int stub_cmd(struct spmi_controller *c,u8 o,u8 s) { (void)c;(void)o;(void)s;return 0; }
#define spmi_read_cmd stub_read
#define spmi_write_cmd stub_write
#define spmi_cmd stub_cmd
static struct irq_domain_ops apple_spmi_irq_domain_ops;
static struct irq_domain *devm_irq_domain_instantiate(struct device *d, struct irq_domain_info *i)
{ (void)d; domain.host_data=i->host_data; domain_limit=i->hwirq_max; return &domain; }
static int devm_add_action(struct device *d, void (*f)(void *), void *p) { (void)d;(void)f;(void)p;return 0; }
static void apple_spmi_irq_handler(struct irq_desc *d) { (void)d; }
static void irq_set_chained_handler_and_data(int i, void (*f)(struct irq_desc *), void *p) { (void)i;(void)f;(void)p; }
static void apple_spmi_teardown_irq(void *p) { (void)p; }
static void raw_spin_lock_init(raw_spinlock_t *p) { (void)p; }
static void bitmap_to_arr32(u32 *d, const unsigned long *s, unsigned int n)
{ unsigned int i; for(i=0;i<n/32;i++) d[i]=(u32)(s[i/(sizeof(unsigned long)/4)] >> ((i%(sizeof(unsigned long)/4))*32)); }
static u32 readl(const void *p) { return *(const volatile u32 *)p; }
#define readl_poll_timeout(addr,val,cond,delay,timeout) ({ (void)(delay);(void)(timeout);(val)=readl(addr);poll_calls++;(cond)?0:-ETIMEDOUT; })
static void writel(u32 v, void *p) { write_offsets[write_count++]=(u32 *)p-(u32 *)active_regs; *(volatile u32 *)p=v; }
static bool pm_resume_via_firmware(void) { return firmware_resume; }
static int apple_spmi_drain_faults(struct device *d, struct apple_spmi *s) { (void)d;(void)s;return 0; }
'''
    harness += declarations + functions
    harness += r'''
int main(void)
{
 u32 regs[1024]={0}, args[2]={2,0}; struct platform_device p={0};
 struct apple_spmi *s; struct irq_fwspec fs={args,2};
 struct irq_domain fake_domain; unsigned long hw; unsigned int type, i;
 active_regs=regs; p.regs=regs; p.irq=7; p.dev.match_data=(void *)&apple_spmi_gen4;
 assert(apple_spmi_probe(&p)==0);
 s=controller.data;
 assert(s->hw->num_irqs==256 && s->hw->irq_mask==0x400 && s->hw->irq_ack==0x600);
 assert(s->irqd && domain.host_data==s && domain_limit==~0U);
 assert(!s->notify_irq && !s->notify_parked);
 assert(apple_spmi_wait_rx_not_empty(&controller)==0 && poll_calls==1);
 for(i=0;i<8;i++) assert(regs[(0x400/4)+i]==0 && regs[(0x600/4)+i]==U32_MAX);
 assert(regs[(0x400/4)+8]==0 && regs[(0x600/4)+8]==U32_MAX);
 fake_domain.host_data=s; assert(apple_spmi_irq_domain_translate(&fake_domain,&fs,&hw,&type)==0 && hw==2);
 args[0]=256; assert(apple_spmi_irq_domain_translate(&fake_domain,&fs,&hw,&type)==-EINVAL); args[0]=2;
 apple_spmi_irq_unmask_raw(s,2); assert(regs[(0x400/4)]==BIT(2));
 apple_spmi_irq_mask_raw(s,2); assert(regs[(0x400/4)]==0);
 apple_spmi_irq_ack_raw(s,2); assert(regs[(0x600/4)]==BIT(2));
 /* Gen4 restores peripheral masks without a parked notification interrupt. */
 s->irq_mask_cache[0]=BIT(2); s->notify_irq=false; s->notify_parked=false;
 regs[0x400/4]=0; write_count=0; firmware_resume=true;
 assert(apple_spmi_resume_noirq(&p.dev)==0);
 assert(regs[0x400/4]==BIT(2) && !s->notify_irq && write_count==8);
 /* Missing IRQ domains and non-firmware resumes must not touch MMIO. */
 s->irqd=NULL; s->notify_parked=false; write_count=0;
 assert(apple_spmi_resume_noirq(&p.dev)==0 && write_count==0);
 s->irqd=&domain; firmware_resume=false; write_count=0;
 assert(apple_spmi_resume_noirq(&p.dev)==0 && write_count==0);
 /* Legacy generations keep their peripheral counts and notification mode. */
 p.dev.match_data=(void *)&apple_spmi_gen1; memset(regs,0,sizeof(regs)); write_count=0;
 assert(apple_spmi_probe(&p)==0); s=controller.data;
 assert(s->hw->num_irqs==256 && s->notify_irq);
 s->irq_mask_cache[0]=BIT(3); s->notify_irq=false; s->notify_parked=true;
 firmware_resume=true; write_count=0;
 assert(apple_spmi_resume_noirq(&p.dev)==0);
 assert(regs[0x20/4]==BIT(3) && s->notify_irq && !s->notify_parked);
 p.dev.match_data=(void *)&apple_spmi_t6000; memset(regs,0,sizeof(regs)); write_count=0;
 assert(apple_spmi_probe(&p)==0); s=controller.data;
 assert(s->hw->num_irqs==192 && s->notify_irq);
 puts("PASS: Apple SPMI probe, IRQ banks, and resume"); return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="apple-spmi-host-") as tmp:
        binary = Path(tmp) / "spmi-host-test"
        command = shlex.split(os.environ.get("CC", "cc")) + ["-std=gnu11", "-Wall", "-Werror", "-O0", "-x", "c", "-o", str(binary), "-"]
        subprocess.run(command, input=harness, text=True, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
