/*
 * Persona 4: Dancing All Night (PCSE00764): its hooks (runtime/build.py RECOMP_PACK_HOOKS), placed by pack.toml's HOOKS=.
 */
#include "hooks.h"
#include <stdio.h>
#include <stdlib.h>

/* CRI's error notifier (CRIWARE's criErr_Notify: r0 the level, r1 an "E<id>:<message>" format, r2.. its arguments), and its
 * one-argument form. P4D_CRI_LOG=1 (a regenerate): each message printed once per caller -- CRI's middleware says why a
 * player, a voice or a file failed here and nowhere else */
static void cri_say(Cpu *c, int nargs) {
	static uint32_t seen[256]; static int nseen;
	uint32_t lr = c->r[14];
	for (int i = 0; i < nseen; i++) if (seen[i] == lr) return;
	if (nseen < 256) seen[nseen++] = lr;
	char msg[256]; uint32_t f = c->r[1]; int n = 0;
	for (; n < 255 && RD8(f + (uint32_t)n); n++) msg[n] = (char)RD8(f + (uint32_t)n);
	msg[n] = 0;
	if (nargs) fprintf(stderr, "    cri: level %u: %s  [arg %#x %d] (lr %#x)\n", c->r[0], msg, c->r[2], (int)c->r[2], lr);
	else fprintf(stderr, "    cri: level %u: %s (lr %#x)\n", c->r[0], msg, lr);
}
void recomp_hook_p4d_cri_err(Cpu *c) { cri_say(c, 0); }
void recomp_hook_p4d_cri_err1(Cpu *c) { cri_say(c, 1); }

/* evtSubTitle.c's update (fn at 0x8103b4a0): the subtitle's frame count += 1 each update, against its length in frames
 * ([task+8]) and its fades (the draw's ratios): written for one update per 1/30 s. At 60 fps (the frame_rate setting)
 * a subtitle ran out in half its time, ahead of the anime cutscene's voices. Here, just after the +1 (r0 = the new
 * count; r4 the task): at 60 fps every other update of a task takes the +1 back, so the count still means 1/30 s.
 * RECOMP_P4D_SUBTITLE30=1: left alone. */
extern int display_frame_rate_60(void);
void recomp_ihook_p4d_subtitle_tick(Cpu *c, int reg) {
	static int off = -1; if (off < 0) off = getenv("RECOMP_P4D_SUBTITLE30") != 0;
	if (off || reg < 0 || reg > 14 || !display_frame_rate_60()) return;
	static uint32_t task[16]; static uint8_t odd[16]; static int next;
	uint32_t t = c->r[4]; int k = 0;
	for (; k < 16 && task[k] != t; k++) {}
	if (k == 16) { k = next; next = (next + 1) & 15; task[k] = t; odd[k] = 0; }
	odd[k] ^= 1;
	if (!odd[k]) c->r[reg] -= 1;   /* (the second of each pair of 1/60 s updates) */
}
