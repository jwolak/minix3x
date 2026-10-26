#define _SYSTEM 1

#include <lib.h>
#include <minix/syslib.h>
#include <minix/sysutil.h>

int _kernel_call(int syscallnr, message *msgptr)
{
    int ticks = 1;
    int call_result = -1;

    while (1) {
        msgptr->m_type = syscallnr;
        do_kernel_call(msgptr);
        call_result = msgptr->m_type;
        if (call_result != ENOTREADY) {
            break;
        }
        tickdelay(ticks++);
    }

    return call_result;
}
