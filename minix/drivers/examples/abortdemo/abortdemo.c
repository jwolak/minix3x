/*-
 * BSD 3-Clause License
 *
 * Copyright (c) 2026, Janusz Wolak
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

/*
 * abortdemo - example system service that shows how to use sys_abort().
 *
 * WARNING: starting this service shuts the whole system down.
 *
 * Usage (as root):
  *   minix-service up /service/abortdemo -config /etc/system.conf.d/abortdemo \
 *       -args "halt|poweroff|reboot"
 *
 * Only system services whose configuration allows the ABORT kernel call
 * can use sys_abort(); see abortdemo.conf. A regular user program cannot
 * call it; user programs use reboot(2), which goes through PM.
 */

#include <minix/drivers.h>
#include <sys/reboot.h>
#include <stdio.h>
#include <string.h>

static int abort_how = RB_HALT;

static int sef_cb_init(int type, sef_init_info_t *UNUSED(info))
{
    int sys_abort_result = -1;

    /* Only act on a fresh start, not after a restart by RS. */
    if (type != SEF_INIT_FRESH)
        return OK;

    printf("abortdemo: calling sys_abort(0x%x)\n", abort_how);

    /*
     * The kernel does not stop at once: it prints a message and shuts down
     * after about one second, so that other processes can finish their work.
     */
    sys_abort_result = sys_abort(abort_how);
    if (sys_abort_result != OK)
        printf("abortdemo: sys_abort failed: %d\n", sys_abort_result);

    return sys_abort_result;
}

int main(int argc, char **argv)
{
    message msg;
    int result = -1;
    int status = -1;

    if (argc > 1) {
        if (strcmp(argv[1], "reboot") == 0)
            abort_how = RB_AUTOBOOT;
        else if (strcmp(argv[1], "poweroff") == 0)
            abort_how = RB_POWERDOWN;
        else if (strcmp(argv[1], "halt") != 0)
            printf("abortdemo: unknown mode '%s', using halt\n", argv[1]);
    }

    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);
    sef_startup();

    /*
     * Keep running until the system goes down; exiting now would make RS
     * restart the service. A HARD_STOP notification arrives before shutdown.
     */
    for (;;) {
        if ((result = sef_receive_status(ANY, &msg, &status)) != OK)
            printf("abortdemo: sef_receive_status failed: %d\n", result);
    }

    return OK;
}
