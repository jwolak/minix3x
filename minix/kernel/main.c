#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <minix/endpoint.h>
#include <machine/vmparam.h>
#include <minix/u64.h>
#include <minix/board.h>
#include <sys/reboot.h>
#include "clock.h"
#include "direct_utils.h"
#include "hw_intr.h"
#include "arch_proto.h"
#include "bsp_bootstrap/bsp_bootstrap.h"

#ifdef CONFIG_SMP
#include "smp.h"
#endif
#ifdef USE_WATCHDOG
#include "watchdog.h"
#endif
#include "spinlock.h"

/* dummy for linking */
char ***_penviron;

/*===========================================================================*
 *			kmain 	                             		*
 *===========================================================================*/
void kmain(kinfo_t *boot_info)
{
    struct boot_image *boot_image_entry; /* current boot image process entry pointer */
    register struct proc *process;       /* process pointer */
    static int bss_area_set_to_zero_check;

    /* BSS sanity check. Check that the BSS area has been set to zero */
    assert(bss_area_set_to_zero_check == 0);
    bss_area_set_to_zero_check = 1;

    if (boot_info == NULL) {
        panic("kmain: boot info pointer is NULL");
    }

    /* Preserve boot parameters and the bootstrap log in kernel-owned globals.
     * Bootstrap memory is reclaimed after initialization. */
    memcpy(&kinfo, boot_info, sizeof(kinfo));   // kinfo - global variable defined in minix/kernel/glo.h
    memcpy(&kmess, kinfo.kmess, sizeof(kmess)); // kmess - global variable defined in minix/kernel/glo.h

    /* Resolve the board name from boot parameters to its numeric ID. */
    machine.board_id = get_board_id_by_name(get_value(kinfo.param_buf, BOARDVARNAME)); // Resolve board name to numeric ID
#ifdef __arm__
    /* We want to initialize serial before we do any output */
    arch_ser_init();
#endif

    DEBUGBASIC(("Minix3x booting...\n"));

    /* Kernel may use bits of main memory before VM is started */
    kernel_may_alloc = 1;

    assert(sizeof(kinfo.boot_procs) == sizeof(image));
    memcpy(kinfo.boot_procs, image, sizeof(kinfo.boot_procs));

    cstart();

    BKL_LOCK();

    DEBUGEXTRA(("main()\n"));

    /* Clear the process table. Anounce each slot as empty and set up mappings
     * for proc_addr() and proc_nr() macros. Do the same for the table with
     * privilege structures for the system processes and the ipc filter pool.
     */
    proc_init();
    IPCF_POOL_INIT();

    if (NR_BOOT_MODULES != kinfo.mbi.mi_mods_count)
        panic("expecting %d boot processes/modules, found %d", NR_BOOT_MODULES, kinfo.mbi.mi_mods_count);

    /* Set up proc table entries for processes in boot image. */
    for (int boot_image_index = 0; boot_image_index < NR_BOOT_PROCS; ++boot_image_index) {
        int schedulable_proc;
        proc_nr_t proc_nr;
        int ipc_to_m, kcalls;
        sys_map_t map;

        boot_image_entry = &image[boot_image_index]; /* process' attributes */
        DEBUGEXTRA(("initializing %s... ", boot_image_entry->proc_name));
        process = proc_addr(boot_image_entry->proc_nr);   /* get process pointer */
        boot_image_entry->endpoint = process->p_endpoint; /* ipc endpoint */
        process->p_cpu_time_left = 0;
        if (boot_image_index < NR_TASKS) /* name (tasks only) */
            strlcpy(process->p_name, boot_image_entry->proc_name, sizeof(process->p_name));

        if (boot_image_index >= NR_TASKS) {
            /* Remember this so it can be passed to VM */
            multiboot_module_t *mb_mod = &kinfo.module_list[boot_image_index - NR_TASKS];
            boot_image_entry->start_addr = mb_mod->mod_start;
            boot_image_entry->len = mb_mod->mod_end - mb_mod->mod_start;
        }

        reset_proc_accounting(process);

        /* See if this process is immediately schedulable.
         * In that case, set its privileges now and allow it to run.
         * Only kernel tasks and the root system process get to run immediately.
         * All the other system processes are inhibited from running by the
         * RTS_NO_PRIV flag. They can only be scheduled once the root system
         * process has set their privileges.
         */
        proc_nr = proc_nr(process);
        schedulable_proc = (iskerneln(proc_nr) || isrootsysn(proc_nr) || proc_nr == VM_PROC_NR);
        if (schedulable_proc) {
            /* Assign privilege structure. Force a static privilege id. */
            (void)get_priv(process, static_priv_id(proc_nr));

            /* Privileges for kernel tasks. */
            if (proc_nr == VM_PROC_NR) {
                priv(process)->s_flags = VM_F;
                priv(process)->s_trap_mask = SRV_T;
                ipc_to_m = SRV_M;
                kcalls = SRV_KC;
                priv(process)->s_sig_mgr = SELF;
                process->p_priority = SRV_Q;
                process->p_quantum_size_ms = SRV_QT;
            } else if (iskerneln(proc_nr)) {
                /* Privilege flags. */
                priv(process)->s_flags = (proc_nr == IDLE ? IDL_F : TSK_F);
                /* Init flags. */
                priv(process)->s_init_flags = TSK_I;
                /* Allowed traps. */
                priv(process)->s_trap_mask = (proc_nr == CLOCK || proc_nr == SYSTEM ? CSK_T : TSK_T);
                ipc_to_m = TSK_M; /* allowed targets */
                kcalls = TSK_KC;  /* allowed kernel calls */
            }
            /* Privileges for the root system process. */
            else {
                assert(isrootsysn(proc_nr));
                priv(process)->s_flags = RSYS_F;     /* privilege flags */
                priv(process)->s_init_flags = SRV_I; /* init flags */
                priv(process)->s_trap_mask = SRV_T;  /* allowed traps */
                ipc_to_m = SRV_M;                    /* allowed targets */
                kcalls = SRV_KC;                     /* allowed kernel calls */
                priv(process)->s_sig_mgr = SRV_SM;   /* signal manager */
                process->p_priority = SRV_Q;         /* priority queue */
                process->p_quantum_size_ms = SRV_QT; /* quantum size */
            }

            /* Fill in target mask. */
            memset(&map, 0, sizeof(map));

            if (ipc_to_m == ALL_M) {
                for (int sys_proc_idx = 0; sys_proc_idx < NR_SYS_PROCS; ++sys_proc_idx) {
                    set_sys_bit(map, sys_proc_idx);
                }
            }

            fill_sendto_mask(process, &map);

            /* Fill in kernel call mask. */
            for (int kernel_call_mask_idx = 0; kernel_call_mask_idx < SYS_CALL_MASK_SIZE; ++kernel_call_mask_idx) {
                priv(process)->s_k_call_mask[kernel_call_mask_idx] = (kcalls == NO_C ? 0 : (~0));
            }
        } else {
            /* Don't let the process run for now. */
            RTS_SET(process, RTS_NO_PRIV | RTS_NO_QUANTUM);
        }

        /* Arch-specific state initialization. */
        arch_boot_proc(boot_image_entry, process);

        /* scheduling functions depend on proc_ptr pointing somewhere. */
        if (!get_cpulocal_var(proc_ptr))
            get_cpulocal_var(proc_ptr) = process;

        /* Process isn't scheduled until VM has set up a pagetable for it. */
        if (process->p_nr != VM_PROC_NR && process->p_nr >= 0) {
            process->p_rts_flags |= RTS_VMINHIBIT;
            process->p_rts_flags |= RTS_BOOTINHIBIT;
        }

        process->p_rts_flags |= RTS_PROC_STOP;
        process->p_rts_flags &= ~RTS_SLOT_FREE;
        DEBUGEXTRA(("done\n"));
    }

    /* update boot procs info for VM */
    memcpy(kinfo.boot_procs, image, sizeof(kinfo.boot_procs));

#define IPCNAME(n)                                                                                                                                             \
    {                                                                                                                                                          \
        assert((n) >= 0 && (n) <= IPCNO_HIGHEST);                                                                                                              \
        assert(!ipc_call_names[n]);                                                                                                                            \
        ipc_call_names[n] = #n;                                                                                                                                \
    }

    arch_post_init();

    IPCNAME(SEND);
    IPCNAME(RECEIVE);
    IPCNAME(SENDREC);
    IPCNAME(NOTIFY);
    IPCNAME(SENDNB);
    IPCNAME(SENDA);

    /* System and processes initialization */
    memory_init();
    DEBUGEXTRA(("system_init()... "));
    system_init();
    DEBUGEXTRA(("done\n"));

    /* The bootstrap phase is over, so we can add the physical
     * memory used for it to the free list.
     */
    add_memmap(&kinfo, kinfo.bootstrap_start, kinfo.bootstrap_len);

#ifdef CONFIG_SMP
    if (config_no_apic) {
        DEBUGBASIC(("APIC disabled, disables SMP, using legacy PIC\n"));
        smp_single_cpu_fallback();
    } else if (config_no_smp) {
        DEBUGBASIC(("SMP disabled, using legacy PIC\n"));
        smp_single_cpu_fallback();
    } else {
        smp_init();
        /*
         * if smp_init() returns it means that it failed and we try to finish
         * single CPU booting
         */
        bsp_finish_booting();
    }
#else
    /*
     * if configured for a single CPU, we are already on the kernel stack which we
     * are going to use everytime we execute kernel code. We finish booting and we
     * never return here
     */
    bsp_finish_booting();
#endif

    NOT_REACHABLE;
}

/*===========================================================================*
 *				prepare_shutdown			     *
 *===========================================================================*/
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

/*===========================================================================*
 *				shutdown 				     *
 *===========================================================================*/
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

/*===========================================================================*
 *				cstart					     *
 *===========================================================================*/
void cstart(void)
{
    /* Perform system initializations prior to calling main(). Most settings are
     * determined with help of the environment strings passed by MINIX' loader.
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

    DEBUGEXTRA(("cstart\n"));

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

/*===========================================================================*
 *				get_value				     *
 *===========================================================================*/

char *get_value(const char *params, /* boot monitor parameters */
                const char *name    /* key to look up */
)
{
    /* Get environment value - kernel version of getenv to avoid setting up the
     * usual environment array.
     */
    register const char *namep;
    register char *envp;

    for (envp = (char *)params; *envp != 0;) {
        for (namep = name; *namep != 0 && *namep == *envp; namep++, envp++)
            ;
        if (*namep == '\0' && *envp == '=')
            return (envp + 1);
        while (*envp++ != 0)
            ;
    }
    return (NULL);
}

/*===========================================================================*
 *				env_get				     	*
 *===========================================================================*/
char *env_get(const char *name) { return get_value(kinfo.param_buf, name); }

void cpu_print_freq(unsigned cpu)
{
    u64_t freq;

    freq = cpu_get_freq(cpu);
    DEBUGBASIC(("CPU %d freq %lu MHz\n", cpu, (unsigned long)(freq / 1000000)));
}

int is_fpu(void) { return get_cpulocal_var(fpu_presence); }
