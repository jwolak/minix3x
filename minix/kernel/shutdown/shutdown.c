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

#include <sys/reboot.h>

#include "kernel/kernel.h"
#include "kernel/clock.h"
#include "direct_utils.h"
#include "hw_intr.h"
#include "shutdown.h"

void prepare_shutdown(const int how)
{
    /* This function prepares to shutdown MINIX. */
    static minix_timer_t shutdown_timer;

    /* Continue after 1 second, to give processes a chance to get scheduled to
     * do shutdown work.  Set a watchog timer to call shutdown(). The timer
     * argument passes the shutdown status.
     */
    printf("MINIX will now be shut down ...\n");
    set_kernel_timer(&shutdown_timer, get_monotonic() + system_hz, minix_shutdown, how);
}

void minix_shutdown(int how)
{
    /* This function is called from prepare_shutdown or stop_sequence to bring
     * down MINIX.
     */

#ifdef CONFIG_SMP
    /*
     * FIXME
     *
     * we will need to stop timers on all cpus if SMP is enabled and put them in
     * such a state that we can perform the whole boot process once restarted from
     * monitor again
     */
    if (ncpus > 1)
        smp_shutdown_aps();
#endif
    hw_intr_disable_all();
    stop_local_timer();

    /* Show shutdown message */
    direct_cls();
    if ((how & RB_POWERDOWN) == RB_POWERDOWN)
        direct_print("MINIX has halted and will now power off.\n");
    else if (how & RB_HALT)
        direct_print("MINIX has halted. "
                     "It is safe to turn off your computer.\n");
    else
        direct_print("MINIX will now reset.\n");
    arch_shutdown(how);
}
