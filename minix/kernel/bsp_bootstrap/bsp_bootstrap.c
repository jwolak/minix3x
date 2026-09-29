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

#include "bsp_bootstrap.h"
#include "kernel/clock.h"
#include "arch_proto.h"
#include "kernel/system_announce/system_announce.h"

#ifdef CONFIG_SMP
#include "kernel/smp.h"
#endif

void bsp_finish_booting(void)
{
	int i;
#if SPROFILE
	sprofiling = 0; /* we're not profiling until instructed to */
#endif			/* SPROFILE */

	cpu_identify();

	vm_running = 0;
	krandom.random_sources = RANDOM_SOURCES;
	krandom.random_elements = RANDOM_ELEMENTS;
	struct system_announce_type announce_instance = system_announce_type.new();

	/* MINIX is now ready. All boot image processes are on the ready queue.
	 * Return to the assembly code to start running the current process.
	 */

	/* it should point somewhere */
	get_cpulocal_var(bill_ptr) = get_cpulocal_var_ptr(idle_proc);
	get_cpulocal_var(proc_ptr) = get_cpulocal_var_ptr(idle_proc);

	announce_instance.display_minix_startup_banner(&announce_instance);

	/*
	 * we have access to the cpu local run queue, only now schedule the processes.
	 * We ignore the slots for the former kernel tasks
	 */
	for (i = 0; i < NR_BOOT_PROCS - NR_TASKS; i++) {
		RTS_UNSET(proc_addr(i), RTS_PROC_STOP);
	}
	/*
	 * Enable timer interrupts and clock task on the boot CPU.  First reset the
	 * CPU accounting values, as the timer initialization (indirectly) uses them.
	 */
	cycles_accounting_init();

	if (boot_cpu_init_timer(system_hz)) {
		panic("FATAL : failed to initialize timer interrupts, "
		      "cannot continue without any clock source!");
	}

	fpu_init();

/* Warnings for sanity checks that take time. These warnings are printed
 * so it's a clear warning no full release should be done with them
 * enabled.
 */
#if DEBUG_SCHED_CHECK
	FIXME("DEBUG_SCHED_CHECK enabled");
#endif
#if DEBUG_VMASSERT
	FIXME("DEBUG_VMASSERT enabled");
#endif
#if DEBUG_PROC_CHECK
	FIXME("PROC check enabled");
#endif

#ifdef CONFIG_SMP
	cpu_set_flag(bsp_cpu_id, CPU_IS_READY);
	machine.processors_count = ncpus;
	machine.bsp_id = bsp_cpu_id;
#else
	machine.processors_count = 1;
	machine.bsp_id = 0;
#endif

	/* Kernel may no longer use bits of memory as VM will be running soon */
	kernel_may_alloc = 0;

	switch_to_user();
	NOT_REACHABLE;
}