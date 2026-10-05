/*-
 * BSD 3-Clause License
 *
 * Copyrights 2026, Janusz Wolak
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <machine/vmparam.h>
#include <stdlib.h>
#include <string.h>
#include "kernel/kernel.h"
#include "kernel/clock.h"
#include "hw_intr.h"

#ifdef CONFIG_SMP
#include "kernel/smp.h"
#endif
#ifdef USE_WATCHDOG
#include "kernel/watchdog.h"
#endif

#include "early_init.h"

void kernel_early_init(void)
{
    /* Perform early system initialization before setting up kernel processes.
     * Most settings are determined from parameters passed by MINIX' loader.
     */
    register char *value; /* value in key=value pair */

    /* low-level initialization */
    prot_init();

    /* determine verbosity */
    if ((value = env_get(VERBOSEBOOTVARNAME)))
        verboseboot = atoi(value);

    /* Initialize clock variables. */
    init_clock();

    /* Get memory parameters. */
    value = env_get("ac_layout");
    if (value && atoi(value)) {
        kinfo.user_sp = (vir_bytes)USR_STACKTOP_COMPACT;
        kinfo.user_end = (vir_bytes)USR_DATATOP_COMPACT;
    }

    DEBUGEXTRA(("kernel_early_init\n"));

    /* Record miscellaneous information for user-space servers. */
    kinfo.nr_procs = NR_PROCS;
    kinfo.nr_tasks = NR_TASKS;
    strlcpy(kinfo.release, OS_RELEASE, sizeof(kinfo.release));
    strlcpy(kinfo.version, OS_VERSION, sizeof(kinfo.version));

    /* Initialize various user-mapped structures. */
    memset(&arm_frclock, 0, sizeof(arm_frclock));

    memset(&kuserinfo, 0, sizeof(kuserinfo));
    kuserinfo.kui_size = sizeof(kuserinfo);
    kuserinfo.kui_user_sp = kinfo.user_sp;

#ifdef USE_APIC
    value = env_get("no_apic");
    if (value)
        config_no_apic = atoi(value);
    else
        config_no_apic = 1;
    value = env_get("apic_timer_x");
    if (value)
        config_apic_timer_x = atoi(value);
    else
        config_apic_timer_x = 1;
#endif

#ifdef USE_WATCHDOG
    value = env_get("watchdog");
    if (value)
        watchdog_enabled = atoi(value);
#endif

#ifdef CONFIG_SMP
    if (config_no_apic)
        config_no_smp = 1;
    value = env_get("no_smp");
    if (value)
        config_no_smp = atoi(value);
    else
        config_no_smp = 0;
#endif
    DEBUGEXTRA(("intr_init(0)\n"));

    intr_init(0);

    arch_init();
}