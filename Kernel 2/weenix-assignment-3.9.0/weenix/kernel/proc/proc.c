/******************************************************************************/
/* Important Fall 2023 CSCI 402 usage information:                            */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "kernel.h"
#include "config.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"
#include "proc/proc.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/mm.h"
#include "mm/mman.h"

#include "vm/vmmap.h"

#include "fs/vfs.h"
#include "fs/vfs_syscall.h"
#include "fs/vnode.h"
#include "fs/file.h"

proc_t *curproc = NULL; /* global */
static slab_allocator_t *proc_allocator = NULL;

static list_t _proc_list;
static proc_t *proc_initproc = NULL; /* Pointer to the init process (PID 1) */

void proc_init()
{
    list_init(&_proc_list);
    proc_allocator = slab_allocator_create("proc", sizeof(proc_t));
    KASSERT(proc_allocator != NULL);
}

proc_t *proc_lookup(int pid)
{
    proc_t *p;
    list_iterate_begin(&_proc_list, p, proc_t, p_list_link)
    {
        if (p->p_pid == pid)
        {
            return p;
        }
    }
    list_iterate_end();
    return NULL;
}

list_t *proc_list()
{
    return &_proc_list;
}

size_t proc_info(const void *arg, char *buf, size_t osize)
{
    const proc_t *p = (proc_t *)arg;
    size_t size = osize;
    proc_t *child;

    KASSERT(NULL != p);
    KASSERT(NULL != buf);

    iprintf(&buf, &size, "pid:          %i\n", p->p_pid);
    iprintf(&buf, &size, "name:         %s\n", p->p_comm);
    if (NULL != p->p_pproc)
    {
        iprintf(&buf, &size, "parent:       %i (%s)\n",
                p->p_pproc->p_pid, p->p_pproc->p_comm);
    }
    else
    {
        iprintf(&buf, &size, "parent:       -\n");
    }

#ifdef __MTP__
    int count = 0;
    kthread_t *kthr;
    list_iterate_begin(&p->p_threads, kthr, kthread_t, kt_plink)
    {
        ++count;
    }
    list_iterate_end();
    iprintf(&buf, &size, "thread count: %i\n", count);
#endif

    if (list_empty(&p->p_children))
    {
        iprintf(&buf, &size, "children:     -\n");
    }
    else
    {
        iprintf(&buf, &size, "children:\n");
    }
    list_iterate_begin(&p->p_children, child, proc_t, p_child_link)
    {
        iprintf(&buf, &size, "     %i (%s)\n", child->p_pid, child->p_comm);
    }
    list_iterate_end();

    iprintf(&buf, &size, "status:       %i\n", p->p_status);
    iprintf(&buf, &size, "state:        %i\n", p->p_state);

#ifdef __VFS__
#ifdef __GETCWD__
    if (NULL != p->p_cwd)
    {
        char cwd[256];
        lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
        iprintf(&buf, &size, "cwd:          %-s\n", cwd);
    }
    else
    {
        iprintf(&buf, &size, "cwd:          -\n");
    }
#endif /* __GETCWD__ */
#endif

#ifdef __VM__
    iprintf(&buf, &size, "start brk:    0x%p\n", p->p_start_brk);
    iprintf(&buf, &size, "brk:          0x%p\n", p->p_brk);
#endif

    return size;
}

size_t proc_list_info(const void *arg, char *buf, size_t osize)
{
    size_t size = osize;
    proc_t *p;

    KASSERT(NULL == arg);
    KASSERT(NULL != buf);

#if defined(__VFS__) && defined(__GETCWD__)
    iprintf(&buf, &size, "%5s %-13s %-18s %-s\n", "PID", "NAME", "PARENT", "CWD");
#else
    iprintf(&buf, &size, "%5s %-13s %-s\n", "PID", "NAME", "PARENT");
#endif

    list_iterate_begin(&_proc_list, p, proc_t, p_list_link)
    {
        char parent[64];
        if (NULL != p->p_pproc)
        {
            snprintf(parent, sizeof(parent),
                     "%3i (%s)", p->p_pproc->p_pid, p->p_pproc->p_comm);
        }
        else
        {
            snprintf(parent, sizeof(parent), "  -");
        }

#if defined(__VFS__) && defined(__GETCWD__)
        if (NULL != p->p_cwd)
        {
            char cwd[256];
            lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
            iprintf(&buf, &size, " %3i  %-13s %-18s %-s\n",
                    p->p_pid, p->p_comm, parent, cwd);
        }
        else
        {
            iprintf(&buf, &size, " %3i  %-13s %-18s -\n",
                    p->p_pid, p->p_comm, parent);
        }
#else
        iprintf(&buf, &size, " %3i  %-13s %-s\n",
                p->p_pid, p->p_comm, parent);
#endif
    }
    list_iterate_end();
    return size;
}

static pid_t next_pid = 0;

/**
 * Returns the next available PID.
 *
 * Note: Where n is the number of running processes, this algorithm is
 * worst case O(n^2). As long as PIDs never wrap around it is O(n).
 *
 * @return the next available PID
 */
static int _proc_getid()
{
    proc_t *p;
    pid_t pid = next_pid;
    while (1)
    {
    failed:
        list_iterate_begin(&_proc_list, p, proc_t, p_list_link)
        {
            if (p->p_pid == pid)
            {
                if ((pid = (pid + 1) % PROC_MAX_COUNT) == next_pid)
                {
                    return -1;
                }
                else
                {
                    goto failed;
                }
            }
        }
        list_iterate_end();
        next_pid = (pid + 1) % PROC_MAX_COUNT;
        return pid;
    }
}

/*
 * The new process, although it isn't really running since it has no
 * threads, should be in the PROC_RUNNING state.
 *
 * Don't forget to set proc_initproc when you create the init
 * process. You will need to be able to reference the init process
 * when reparenting processes to the init process.
 */

// FOR ALL FUNCTIONS, ADD KASSERT, DBG STATEMENTS (READ ABOUT TESTING IN GUIDELINES)
// VFS AND VM TOO???
// BUT ESPECIALLY DBG AND KASSERT!!!!
proc_t *proc_create(char *name)
{
    // Start creating the process. For each 'field' in proc.h,
    // initialize them
    proc_t *process;

    process = (proc_t *)slab_obj_alloc(proc_allocator);
    process->p_pid = _proc_getid(); // Process ID (PID)

    KASSERT(PID_IDLE != process->p_pid || list_empty(&_proc_list)); // Can only be idle if
    // this is the first process

    KASSERT(PID_INIT != process->p_pid || PID_IDLE == curproc->p_pid); // PID can only be init
    // if creating from the IDLE process
    dbg(DBG_PRINT, "(GRADING1A 2.a)\n");
    strcpy(process->p_comm, name);   // Process Name
    list_init(&process->p_threads);  // Process' threads list
    list_init(&process->p_children); // Process' children list

    // Queue the process puts itself in when it
    // wants to wait on one of its children. See proc.h
    sched_queue_init(&process->p_wait);
    //
    process->p_pagedir = pt_create_pagedir();
    process->p_status = 0;           // process status
    process->p_state = PROC_RUNNING; // process state initialized as running

    // Initialize for structs of type list_link_t
    list_link_init(&process->p_child_link);
    list_link_init(&process->p_list_link);
    list_insert_tail(&_proc_list, &process->p_list_link);

    if (process->p_pid == PID_IDLE)
    {
        // In this case (IDLE), set out parent process to NULL
        process->p_pproc = NULL;
        dbg(DBG_PRINT, "(GRADING1A)\n");
        return process;
    }
    else if (process->p_pid == PID_INIT)
    {
        // In the case where our PID is 1...
        // Try and understand a bit better...
        proc_initproc = process;
        process->p_pproc = curproc;
        list_insert_tail(&curproc->p_children, &process->p_child_link);
        dbg(DBG_PRINT, "(GRADING1A)\n");
        return process;
    }
    else
    {
        // Else...
        process->p_pproc = curproc;
        list_insert_tail(&curproc->p_children, &process->p_child_link);
        dbg(DBG_PRINT, "(GRADING1A)\n");
        return process;
    }
}

/**
 * Cleans up as much as the process as can be done from within the
 * process. This involves:
 *    - Closing all open files (VFS)
 *    - Cleaning up VM mappings (VM)
 *    - Waking up its parent if it is waiting
 *    - Reparenting any children to the init process
 *    - Setting its status and state appropriately
 *
 * The parent will finish destroying the process within do_waitpid (make
 * sure you understand why it cannot be done here). Until the parent
 * finishes destroying it, the process is informally called a 'zombie'
 * process.
 *
 * This is also where any children of the current process should be
 * reparented to the init process (unless, of course, the current
 * process is the init process. However, the init process should not
 * have any children at the time it exits).
 *
 * Note: You do _NOT_ have to special case the idle process. It should
 * never exit this way.
 *
 * @param status the status to exit the process with
 */
void proc_cleanup(int status)
{
    KASSERT(NULL != proc_initproc);
    KASSERT(1 <= curproc->p_pid);
    KASSERT(NULL != curproc->p_pproc);
    dbg(DBG_PRINT, "(GRADING1A 2.b)\n");
    proc_t *childProcess;
    // Iterate over the current process's children
    // and make each child's parent = proc_initproc (PID 1)
    if (!list_empty(&(curproc->p_children)))
    {
        list_iterate_begin(&(curproc->p_children), childProcess, proc_t, p_child_link)
        {
            childProcess->p_pproc = proc_initproc;
            list_insert_tail(&(proc_initproc->p_children), &(childProcess->p_child_link));
            dbg(DBG_PRINT, "(GRADING1C)\n");
        }
        list_iterate_end();
        dbg(DBG_PRINT, "(GRADING1C)\n");
    }

    curproc->p_state = PROC_DEAD;
    curproc->p_status = status;

    if (!sched_queue_empty(&(curproc->p_pproc->p_wait)))
    {
        dbg(DBG_PRINT, "(GRADING1C)\n");
        sched_broadcast_on(&(curproc->p_pproc->p_wait));
    }

    KASSERT(NULL != curproc->p_pproc);
    KASSERT(KT_EXITED == curthr->kt_state);
    dbg(DBG_PRINT, "(GRADING1A 2.b)\n");
}

/*
 * This has nothing to do with signals and kill(1).
 *
 * Calling this on the current process is equivalent to calling
 * do_exit().
 *
 * In Weenix, this is only called from proc_kill_all.
 */
void proc_kill(proc_t *p, int status)
{
    if (p == curproc)
    {
        dbg(DBG_PRINT, "(GRADING1C)\n");
        do_exit(status);
    }
    else
    {
        kthread_t *childThread;
        list_iterate_begin(&p->p_threads, childThread, kthread_t, kt_plink)
        {
            dbg(DBG_PRINT, "(GRADING1C)\n");
            childThread->kt_cancelled = 1;
            kthread_cancel(childThread, (void *)status);
        }
        list_iterate_end();
        dbg(DBG_PRINT, "(GRADING1C)\n");
    }

    // NOT_YET_IMPLEMENTED("PROCS: proc_kill");
}

/*
 * Remember, proc_kill on the current process will _NOT_ return.
 * Don't kill direct children of the idle process.
 *
 * In Weenix, this is only called by sys_halt.
 */
void proc_kill_all()
{
    proc_t *process;
    dbg(DBG_PRINT, "(GRADING1C)\n");
    list_iterate_begin(&_proc_list, process, proc_t, p_list_link)
    {
        if (process->p_pid != PID_IDLE && process->p_pproc->p_pid != PID_IDLE)
        {
            dbg(DBG_PRINT, "(GRADING1C)\n");
            // if the process is not IDLE process or its direct child
            proc_kill(process, 0);
        }
        dbg(DBG_PRINT, "(GRADING1C)\n");
    }
    list_iterate_end();
}

/*
 * This function is only called from kthread_exit.
 *
 * Unless you are implementing MTP, this just means that the process
 * needs to be cleaned up and a new thread needs to be scheduled to
 * run. If you are implementing MTP, a single thread exiting does not
 * necessarily mean that the process should be exited.
 */
void proc_thread_exited(void *retval)
{
    // NOT_YET_IMPLEMENTED("PROCS: proc_thread_exited");
    dbg(DBG_PRINT, "(GRADING1A)\n");
    proc_cleanup((int)retval);
    sched_switch();
}

/*
 * This function is only called from do_waitpid.
 */
pid_t _waitpid_helper1(pid_t pid, int *status)
{

    proc_t *process;         // definition of process control block is in "/include/proc/proc.h"
    kthread_t *child_thread; // definition of process control block is in "/include/proc/kthread.h"
    pid_t return_pid;        // pid_t = int32_t

    // to wait for one of the child process to die, we are using while loop
    while (1)
    {

        list_iterate_begin(&curproc->p_children, process, proc_t, p_child_link)
        {

            dbg(DBG_PRINT, "(GRADING1A)\n");
            // PROC_DEAD is an enum 'proc_state_t' in "proc.h"
            if (process->p_state == PROC_DEAD)
            {
                /*
                 if the process is dead, we communicate exit status via 'status' variable
                 and remove all child threads for this current dead process
                */
                if (status != NULL)
                {
                    dbg(DBG_PRINT, "(GRADING1C)\n");
                    *status = process->p_status;
                }

                return_pid = process->p_pid;

                // cleanup current process's threads
                list_iterate_begin(&process->p_threads, child_thread, kthread_t, kt_plink)
                {
                    dbg(DBG_PRINT, "(GRADING1A)\n");
                    kthread_destroy(child_thread);
                    dbg(DBG_PRINT, "(GRADING1C)\n");
                }
                list_iterate_end();

                list_remove(&process->p_list_link);  // removing current 'process' from the list of all processes
                list_remove(&process->p_child_link); // removing current 'process' from the list of all child processes

                KASSERT(NULL != process);
                KASSERT(-1 == pid || process->p_pid == pid);
                KASSERT(NULL != process->p_pagedir);
                dbg(DBG_PRINT, "(GRADING1A 2.c)\n");
                return return_pid;
            }
        }
        list_iterate_end();
        dbg(DBG_PRINT, "(GRADING1A)\n");
        sched_sleep_on(&curproc->p_wait); // sleep till one of the processes in 'p_wait' queue exists
    }
}

/*
 * This function is only called from do_waitpid.
 */
pid_t _waitpid_helper2(pid_t pid, int *status)
{

    proc_t *process;         // definition of process control block is in "/include/proc/proc.h"
    kthread_t *child_thread; // definition of process control block is in "/include/proc/kthread.h"
    pid_t return_pid;        // pid_t = int32_t
    dbg(DBG_PRINT, "(GRADING1A)\n");
    list_iterate_begin(&curproc->p_children, process, proc_t, p_child_link)
    {

        if (process->p_pid == pid)
        {
            // if the current element in the list has same pid as passed by user as argument
            dbg(DBG_PRINT, "(GRADING1A)\n");
            while (process->p_state != PROC_DEAD)
            {
                // wait till this process with 'pid' is not dead
                dbg(DBG_PRINT, "(GRADING1A)\n");
                sched_sleep_on(&curproc->p_wait);
                dbg(DBG_PRINT, "(GRADING1C)\n");
            }

            if (status != NULL)
            {
                *status = process->p_status;
            }

            return_pid = process->p_pid;

            // cleanup current process's threads (the one with pid)
            list_iterate_begin(&process->p_threads, child_thread, kthread_t, kt_plink)
            {
                dbg(DBG_PRINT, "(GRADING1A)\n");
                kthread_destroy(child_thread);
                dbg(DBG_PRINT, "(GRADING1C)\n");
            }
            list_iterate_end();

            list_remove(&process->p_list_link);  // removing current 'process' from the list of all processes
            list_remove(&process->p_child_link); // removing current 'process' from the list of all child processes

            KASSERT(process != NULL);
            KASSERT(pid == -1 || process->p_pid == pid);
            KASSERT(NULL != process->p_pagedir);
            dbg(DBG_PRINT, "(GRADING1A 2.c)\n");
            return return_pid;
        }
    }
    list_iterate_end();
    dbg(DBG_PRINT, "(GRADING1A)\n");
    // if process with pid, has no children, then return -ECHILD
    return -ECHILD;
}

/* If pid is -1 dispose of one of the exited children of the current
 * process and return its exit status in the status argument, or if
 * all children of this process are still running, then this function
 * blocks on its own p_wait queue until one exits.
 *
 * If pid is greater than 0 and the given pid is a child of the
 * current process then wait for the given pid to exit and dispose
 * of it.
 *
 * If the current process has no children, or the given pid is not
 * a child of the current process return -ECHILD.
 *
 * Pids other than -1 and positive numbers are not supported.
 * Options other than 0 are not supported.
 */
pid_t do_waitpid(pid_t pid, int options, int *status)
{
    dbg(DBG_PRINT, "(GRADING1A)\n");
    if (list_empty(&curproc->p_children))
    {
        dbg(DBG_PRINT, "(GRADING1A)\n");
        return -ECHILD; // returning -10 as per "weenix/user/include/errno.h"
    }

    if (pid == -1)
    {
        // pick one of the dead processes and return its status
        return _waitpid_helper1(pid, status);
    }
    else if (pid > 0)
    {
        // pick the process with passed pid and wait for its child to be dead and return its status
        return _waitpid_helper2(pid, status);
    }

    // pids other than -1 or > 0 are not supported
    return 0; // this line will be executed if we have an integer <= -2
}

/*
 * Cancel all threads and join with them (if supporting MTP), and exit from the current
 * thread.
 *
 * @param status the exit status of the process
 */
void do_exit(int status)
{

    kthread_t *thread; // kthread_t struct definition in "include/proc/kthread.h"
    // thread var will be used to store the kernel thread block info, during the iteration
    list_iterate_begin(&curproc->p_threads, thread, kthread_t, kt_plink)
    {
        thread->kt_cancelled = 1;
        dbg(DBG_PRINT, "(GRADING1C)\n");
    }
    list_iterate_end();
    dbg(DBG_PRINT, "(GRADING1C)\n");
    kthread_exit((void *)status);
}