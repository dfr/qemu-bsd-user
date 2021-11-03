/*
 *  arm signal definitions
 *
 *  Copyright (c) 2013 Stacey D. Son
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _TARGET_ARCH_SIGNAL_H_
#define _TARGET_ARCH_SIGNAL_H_

#include "cpu.h"

#define TARGET_REG_R0   0
#define TARGET_REG_R1   1
#define TARGET_REG_R2   2
#define TARGET_REG_R3   3
#define TARGET_REG_R4   4
#define TARGET_REG_R5   5
#define TARGET_REG_R6   6
#define TARGET_REG_R7   7
#define TARGET_REG_R8   8
#define TARGET_REG_R9   9
#define TARGET_REG_R10  10
#define TARGET_REG_R11  11
#define TARGET_REG_R12  12
#define TARGET_REG_R13  13
#define TARGET_REG_R14  14
#define TARGET_REG_R15  15
#define TARGET_REG_CPSR 16
#define TARGET__NGREG   17
/* Convenience synonyms */
#define TARGET_REG_FP   TARGET_REG_R11
#define TARGET_REG_SP   TARGET_REG_R13
#define TARGET_REG_LR   TARGET_REG_R14
#define TARGET_REG_PC   TARGET_REG_R15

#define TARGET_INSN_SIZE    4       /* arm instruction size */

/* Size of the signal trampolin code. See _sigtramp(). */
#define TARGET_SZSIGCODE    ((abi_ulong)(9 * TARGET_INSN_SIZE))

/* compare to arm/include/_limits.h */
#define TARGET_MINSIGSTKSZ  (1024 * 4)                  /* min sig stack size */
#define TARGET_SIGSTKSZ     (TARGET_MINSIGSTKSZ + 32768)  /* recommended size */

/*
 * Floating point register state
 */
typedef struct target_mcontext_vfp {
    abi_ullong  mcv_reg[32];
    abi_ulong   mcv_fpscr;
} target_mcontext_vfp_t;

typedef struct target_mcontext {
    uint32_t    __gregs[32];

    /*
     * Originally, rest of this structure was named __fpu, 35 * 4 bytes
     * long, never accessed from kernel.
     */
    abi_long    mc_vfp_size;
    abi_ptr     *mc_vfp_ptr;
    abi_int     mc_spare[33];
} target_mcontext_t;

#include "target_os_ucontext.h"

struct target_sigframe {
    target_siginfo_t    sf_si;  /* saved siginfo */
    target_ucontext_t   sf_uc;  /* saved ucontext */
    target_mcontext_vfp_t sf_vfp; /* actual saved VFP context */
};

/*
 * Compare to arm/arm/machdep.c sendsig()
 * Assumes that target stack frame memory is locked.
 */
static inline abi_long
set_sigtramp_args(CPUARMState *env, int sig, struct target_sigframe *frame,
    abi_ulong frame_addr, struct target_sigaction *ka)
{
    abi_ulong cpsr;

    /*
     * Arguments to signal handler:
     *  r0 = signal number
     *  r1 = siginfo pointer
     *  r2 = ucontext pointer
     *  r5 = ucontext pointer
     *  pc = signal handler pointer
     *  sp = sigframe struct pointer
     *  lr = sigtramp at base of user stack
     */

    env->regs[0] = sig;
    env->regs[1] = frame_addr +
        offsetof(struct target_sigframe, sf_si);
    env->regs[2] = frame_addr +
        offsetof(struct target_sigframe, sf_uc);

    /* the trampoline uses r5 as the uc address */
    env->regs[5] = frame_addr +
        offsetof(struct target_sigframe, sf_uc);
    env->regs[TARGET_REG_PC] = ka->_sa_handler & ~1;
    env->regs[TARGET_REG_SP] = frame_addr;
    env->regs[TARGET_REG_LR] = TARGET_PS_STRINGS - TARGET_SZSIGCODE;
    /*
     * Low bit indicates whether or not we're entering thumb mode.
     */
    cpsr_write(env, (ka->_sa_handler & 1) * CPSR_T, CPSR_T, CPSRWriteByInstr);

    return 0;
}

/*
 * Compare to arm/arm/machdep.c get_mcontext()
 * Assumes that the memory is locked if mcp points to user memory.
 */
static inline abi_long get_mcontext(CPUARMState *env, target_mcontext_t *mcp,
        int flags)
{
    int err = 0;
    uint32_t *gr = mcp->__gregs;

    if (mcp->mc_vfp_size != 0 && mcp->mc_vfp_size != sizeof(target_mcontext_vfp_t)) {
        return -TARGET_EINVAL;
    }

    gr[TARGET_REG_CPSR] = tswap32(cpsr_read(env));
    if (flags & TARGET_MC_GET_CLEAR_RET) {
        gr[TARGET_REG_R0] = 0;
        gr[TARGET_REG_CPSR] &= ~CPSR_C;
    } else {
        gr[TARGET_REG_R0] = tswap32(env->regs[0]);
    }

    gr[TARGET_REG_R1] = tswap32(env->regs[1]);
    gr[TARGET_REG_R2] = tswap32(env->regs[2]);
    gr[TARGET_REG_R3] = tswap32(env->regs[3]);
    gr[TARGET_REG_R4] = tswap32(env->regs[4]);
    gr[TARGET_REG_R5] = tswap32(env->regs[5]);
    gr[TARGET_REG_R6] = tswap32(env->regs[6]);
    gr[TARGET_REG_R7] = tswap32(env->regs[7]);
    gr[TARGET_REG_R8] = tswap32(env->regs[8]);
    gr[TARGET_REG_R9] = tswap32(env->regs[9]);
    gr[TARGET_REG_R10] = tswap32(env->regs[10]);
    gr[TARGET_REG_R11] = tswap32(env->regs[11]);
    gr[TARGET_REG_R12] = tswap32(env->regs[12]);

    gr[TARGET_REG_SP] = tswap32(env->regs[13]);
    gr[TARGET_REG_LR] = tswap32(env->regs[14]);
    gr[TARGET_REG_PC] = tswap32(env->regs[15]);

    if (mcp->mc_vfp_size != 0 && mcp->mc_vfp_ptr != NULL) {
        /* see get_vfpcontext in sys/arm/arm/exec_machdep.c */
        target_mcontext_vfp_t *vfp = (target_mcontext_vfp_t *)mcp->mc_vfp_ptr;
        for (int i = 0; i < 32; i++) {
            vfp->mcv_reg[i] = tswap64(*aa32_vfp_dreg(env, i));
        }
        vfp->mcv_fpscr = tswap32(vfp_get_fpscr(env));
    }
    return err;
}

/* Compare to arm/arm/exec_machdep.c set_mcontext() */
static inline abi_long set_mcontext(CPUARMState *env, target_mcontext_t *mcp,
        int srflag)
{
    int err = 0;
    const uint32_t *gr = mcp->__gregs;
    uint32_t cpsr, ccpsr = cpsr_read(env);
    uint32_t fpscr;

    cpsr = tswap32(gr[TARGET_REG_CPSR]);
    /*
     * Only allow certain bits to change, reject attempted changes to non-user
     * bits. In addition, make sure we're headed for user mode and none of the
     * interrupt bits are set.
     */
    if ((ccpsr & ~CPSR_USER) != (cpsr & ~CPSR_USER)) {
        return -TARGET_EINVAL;
    }
    if ((cpsr & CPSR_M) != ARM_CPU_MODE_USR ||
        (cpsr & (CPSR_I | CPSR_F)) != 0) {
        return -TARGET_EINVAL;
    }

    /*
     * Make sure that we either have no vfp, or it's the correct size.
     * FreeBSD just ignores it, though, so maybe we'll need to adjust
     * things below instead.
     */
    if (mcp->mc_vfp_size != 0 && mcp->mc_vfp_size != sizeof(target_mcontext_vfp_t)) {
        return -TARGET_EINVAL;
    }
    /*
     * Make sure T mode matches the PC's notion of thumb mode, although
     * FreeBSD lets the processor sort this out, so we may need remove
     * this check, or generate a signal...
     */
    if (!!(tswap32(gr[TARGET_REG_PC]) & 1) != !!(cpsr & CPSR_T)) {
        return -TARGET_EINVAL;
    }

    env->regs[0] = tswap32(gr[TARGET_REG_R0]);
    env->regs[1] = tswap32(gr[TARGET_REG_R1]);
    env->regs[2] = tswap32(gr[TARGET_REG_R2]);
    env->regs[3] = tswap32(gr[TARGET_REG_R3]);
    env->regs[4] = tswap32(gr[TARGET_REG_R4]);
    env->regs[5] = tswap32(gr[TARGET_REG_R5]);
    env->regs[6] = tswap32(gr[TARGET_REG_R6]);
    env->regs[7] = tswap32(gr[TARGET_REG_R7]);
    env->regs[8] = tswap32(gr[TARGET_REG_R8]);
    env->regs[9] = tswap32(gr[TARGET_REG_R9]);
    env->regs[10] = tswap32(gr[TARGET_REG_R10]);
    env->regs[11] = tswap32(gr[TARGET_REG_R11]);
    env->regs[12] = tswap32(gr[TARGET_REG_R12]);

    env->regs[13] = tswap32(gr[TARGET_REG_SP]);
    env->regs[14] = tswap32(gr[TARGET_REG_LR]);
    env->regs[15] = tswap32(gr[TARGET_REG_PC]);
    if (mcp->mc_vfp_size != 0 && mcp->mc_vfp_ptr != NULL) {
        /* see set_vfpcontext in sys/arm/arm/exec_machdep.c */
        target_mcontext_vfp_t *vfp = (target_mcontext_vfp_t *)mcp->mc_vfp_ptr;
        for (int i = 0; i < 32; i++) {
            *aa32_vfp_dreg(env, i) = tswap64(vfp->mcv_reg[i]);
        }
        fpscr =  tswap32(vfp->mcv_fpscr);
        vfp_set_fpscr(env, fpscr);
        /*
         * linux-user sets fpexc, fpinst and fpinst2, but these aren't in
         * FreeBSD's mcontext, what to do?
         */
    }
    cpsr_write(env, cpsr, CPSR_USER | CPSR_EXEC, CPSRWriteByInstr);

    return err;
}

/* Compare to arm/arm/machdep.c sys_sigreturn() */
static inline abi_long get_ucontext_sigreturn(CPUARMState *env,
        abi_ulong target_sf, abi_ulong *target_uc)
{
    *target_uc = target_sf;

    return 0;
}

#endif /* !_TARGET_ARCH_SIGNAL_H_ */
