/*
 * Concurrent Programming & POSIX Scheduling
 *
 * DESCRIPTION:
 *   Simulates three critical flight control tasks running in parallel:
 *     1. Stability Control  (CRITICAL  - highest priority: 80)
 *     2. Navigation         (MEDIUM    - medium priority:  40)
 *     3. Telemetry          (LOW       - lowest priority:  10)
 *
 * SUPPORTED TEST CASES:
 *   Case A - Normal      : SCHED_OTHER (default Linux scheduler)
 *   Case B - Real-Time   : SCHED_FIFO  (strict priority pre-emption)
 *   Case C - Prio Inver. : SCHED_FIFO  + shared mutex to demonstrate
 *                          priority inversion (low task holds mutex
 *                          that high-priority task needs)
 *
 * COMPILATION:
 *   gcc -Wall -O0 -o flight_sim flight_control_sim.c -lpthread -lrt -lm
 *
 * EXECUTION:
 *   Case A:  ./flight_sim A
 *   Case B:  sudo ./flight_sim B
 *   Case C:  sudo ./flight_sim C
 *
 * NOTES:
 *   - Cases B and C require root privileges (sudo) to set real-time
 *     scheduling policies via sched_setscheduler / pthread_setschedparam.
 *   - The telemetry thread uses a POSIX interval timer (timer_create)
 *     instead of sleep(), providing more precise 500 ms activations.
 *   - All output strings and identifiers are in English as requested.
 * ============================================================================
 */

/* ── Feature-test macros ────────────────────────────────────────────────────
 * _GNU_SOURCE  : Exposes POSIX extensions (CPU_SET, sched_*, etc.) on Linux.
 * _POSIX_C_SOURCE 199309L : Ensures POSIX.1b real-time features are visible
 *                           (timer_create, sigevent, SIGRTMIN, etc.).
 * ─────────────────────────────────────────────────────────────────────────── */
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L

/* ── Standard headers ───────────────────────────────────────────────────── */
#include <stdio.h>       /* printf, fprintf                                  */
#include <stdlib.h>      /* exit, atoi                                       */
#include <string.h>      /* memset, strcmp                                   */
#include <unistd.h>      /* getpid                                           */
#include <math.h>        /* sin, cos, sqrt  (heavy-work simulation)          */
#include <time.h>        /* clock_gettime, timer_create, struct timespec     */
#include <signal.h>      /* sigaction, sigevent, SIGRTMIN                    */
#include <errno.h>       /* errno, strerror                                  */

/* ── POSIX threading header ─────────────────────────────────────────────── */
#include <pthread.h>     /* pthread_t, pthread_create, pthread_attr_t, …    */

/* ── Real-time / scheduling headers ────────────────────────────────────── */
#include <sched.h>       /* SCHED_FIFO, SCHED_OTHER, sched_param            */

/* ══════════════════════════════════════════════════════════════════════════
 *  CONSTANTS & CONFIGURATION
 * ══════════════════════════════════════════════════════════════════════════ */

#define EXPERIMENT_DURATION_SEC   10      /* Total measurement window (s)   */
#define TELEMETRY_PERIOD_MS       500     /* Telemetry timer period (ms)    */

/* Real-time priorities (valid range for SCHED_FIFO: 1-99)                  */
#define PRIO_STABILITY   80
#define PRIO_NAVIGATION  40
#define PRIO_TELEMETRY   10

/* Number of loop iterations per "chunk" inside the heavy-work function.
 * Larger → each iteration takes longer → fewer iterations in 10 s.         */
#define HEAVY_WORK_ITERS 200000UL

/* Signal used by the POSIX timer to wake the telemetry thread              */
#define TELEMETRY_SIGNAL SIGRTMIN

/* ══════════════════════════════════════════════════════════════════════════
 *  DATA TYPES
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * TestCase - selects which scheduling scenario to run.
 */
typedef enum {
    CASE_A_NORMAL   = 0,   /* SCHED_OTHER - no special priority             */
    CASE_B_REALTIME = 1,   /* SCHED_FIFO  - strict RT priorities            */
    CASE_C_PRIOINV  = 2    /* SCHED_FIFO  + intentional priority inversion   */
} TestCase;

/*
 * ThreadArgs - passed to each worker thread at creation time.
 *   name       : human-readable label for logging
 *   priority   : real-time priority (used in Cases B and C)
 *   policy     : SCHED_FIFO or SCHED_OTHER
 *   counter    : iteration counter (written by thread, read by main)
 *   test_case  : which scenario is active
 */
typedef struct {
    const char  *name;
    int          priority;
    int          policy;
    volatile long counter;   /* volatile: read from another thread (main)   */
    TestCase     test_case;
} ThreadArgs;

/* ══════════════════════════════════════════════════════════════════════════
 *  GLOBAL VARIABLES
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * g_running - flag shared between all threads and main().
 *   Set to 1 at startup; set to 0 when EXPERIMENT_DURATION_SEC elapses.
 *   Declared volatile because it is written by main() and read by threads.
 */
static volatile int g_running = 1;

/*
 * g_shared_mutex - used ONLY in Case C to demonstrate priority inversion.
 *   The low-priority telemetry thread acquires it first; the high-priority
 *   stability thread then tries to acquire it and blocks → inversion occurs.
 */
static pthread_mutex_t g_shared_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * g_telemetry_timer - POSIX interval timer that fires every 500 ms.
 *   Delivers TELEMETRY_SIGNAL to the telemetry thread's signal mask.
 */
static timer_t g_telemetry_timer;

/*
 * g_telemetry_tid - thread ID of the telemetry thread.
 *   Needed so the timer can send the signal to exactly that thread via
 *   sigev_notify = SIGEV_THREAD_ID (Linux-specific).
 */
static pthread_t g_telemetry_tid;

/* Thread argument structs (global so threads can be joined in main)        */
static ThreadArgs args_stability;
static ThreadArgs args_navigation;
static ThreadArgs args_telemetry;

/* ══════════════════════════════════════════════════════════════════════════
 *  HELPER: apply_rt_scheduling()
 *
 *  Applies the requested scheduling policy and priority to the calling
 *  thread using pthread_setschedparam().
 *
 *  Parameters:
 *    policy   : SCHED_FIFO or SCHED_OTHER
 *    priority : real-time priority (1-99 for FIFO, ignored for OTHER)
 *    name     : thread name (only for error messages)
 * ══════════════════════════════════════════════════════════════════════════ */
static void apply_rt_scheduling(int policy, int priority, const char *name)
{
    struct sched_param sp;
    memset(&sp, 0, sizeof(sp));

    /* For SCHED_OTHER the priority field must be 0 (Linux requirement).    */
    sp.sched_priority = (policy == SCHED_FIFO) ? priority : 0;

    int ret = pthread_setschedparam(pthread_self(), policy, &sp);
    if (ret != 0) {
        /*
         * Most common reason for failure: insufficient privileges.
         * SCHED_FIFO requires CAP_SYS_NICE (i.e., root or sudo).
         */
        fprintf(stderr,
                "[WARNING] %s: pthread_setschedparam failed: %s\n"
                "          → Run with sudo for real-time scheduling.\n",
                name, strerror(ret));
    } else {
        printf("[SCHED]  %-20s policy=%s  priority=%d\n",
               name,
               (policy == SCHED_FIFO) ? "SCHED_FIFO " : "SCHED_OTHER",
               priority);
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 *  HELPER: heavy_work()
 *
 *  Simulates CPU-intensive computation to consume real CPU cycles.
 *  Returns a dummy result so the compiler cannot optimize the loop away.
 *
 *  Uses transcendental math functions (sin/cos/sqrt) because they are
 *  costly and not easily eliminated by the optimizer even at -O2.
 * ══════════════════════════════════════════════════════════════════════════ */
static double heavy_work(void)
{
    volatile double result = 0.0;   /* volatile prevents dead-store removal */
    for (unsigned long i = 0; i < HEAVY_WORK_ITERS; i++) {
        result += sin((double)i) * cos((double)i) + sqrt((double)(i + 1));
    }
    return result;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  THREAD FUNCTION: stability_thread()
 *
 *  Represents the flight stability control task.
 *  Priority: PRIO_STABILITY (80) — highest.
 *
 *  In Case C it also tries to acquire g_shared_mutex, which the telemetry
 *  thread (priority 10) may already hold → classic priority inversion.
 * ══════════════════════════════════════════════════════════════════════════ */
static void *stability_thread(void *arg)
{
    ThreadArgs *ta = (ThreadArgs *)arg;

    /* Apply scheduling policy for this thread                              */
    apply_rt_scheduling(ta->policy, ta->priority, ta->name);

    printf("[THREAD] %s started (PID=%d)\n", ta->name, (int)getpid());

    while (g_running) {
        /* ── Case C: attempt to acquire shared mutex ─────────────────────
         * When the telemetry thread (low priority) holds this mutex, the
         * stability thread (high priority) is BLOCKED waiting for it.
         * This is the definition of priority inversion: a high-priority
         * task is forced to wait for a low-priority task to release a lock.
         * ──────────────────────────────────────────────────────────────── */
        if (ta->test_case == CASE_C_PRIOINV) {
            pthread_mutex_lock(&g_shared_mutex);
        }

        /* ── Simulate flight stability control computation ─────────────── */
        (void)heavy_work();
        ta->counter++;

        if (ta->test_case == CASE_C_PRIOINV) {
            pthread_mutex_unlock(&g_shared_mutex);
        }
    }

    printf("[THREAD] %s finished. Iterations: %ld\n", ta->name, ta->counter);
    return NULL;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  THREAD FUNCTION: navigation_thread()
 *
 *  Represents the navigation / route-processing task.
 *  Priority: PRIO_NAVIGATION (40) — medium.
 * ══════════════════════════════════════════════════════════════════════════ */
static void *navigation_thread(void *arg)
{
    ThreadArgs *ta = (ThreadArgs *)arg;

    apply_rt_scheduling(ta->policy, ta->priority, ta->name);

    printf("[THREAD] %s started (PID=%d)\n", ta->name, (int)getpid());

    while (g_running) {
        /* Simulate route calculation (GPS data processing, etc.)           */
        (void)heavy_work();
        ta->counter++;
    }

    printf("[THREAD] %s finished. Iterations: %ld\n", ta->name, ta->counter);
    return NULL;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  THREAD FUNCTION: telemetry_thread()
 *
 *  Represents the telemetry / ground-station downlink task.
 *  Priority: PRIO_TELEMETRY (10) — lowest.
 *
 *  Instead of using sleep(), this thread BLOCKS on sigwaitinfo() waiting
 *  for TELEMETRY_SIGNAL, which is delivered by the POSIX interval timer
 *  every exactly 500 ms.  This is more deterministic than sleep() because:
 *    • The kernel wakes the thread precisely when the timer fires.
 *    • No accumulation of sleep drift over multiple periods.
 *
 *  In Case C, this thread holds g_shared_mutex for an extended period
 *  while performing heavy work, causing priority inversion.
 * ══════════════════════════════════════════════════════════════════════════ */
static void *telemetry_thread(void *arg)
{
    ThreadArgs *ta = (ThreadArgs *)arg;

    apply_rt_scheduling(ta->policy, ta->priority, ta->name);

    printf("[THREAD] %s started (PID=%d)\n", ta->name, (int)getpid());

    /* ── Set up signal mask so this thread receives TELEMETRY_SIGNAL ──────
     * We block the signal in the signal mask of this thread, then use
     * sigwaitinfo() to synchronously dequeue it.  This avoids installing
     * an asynchronous signal handler (which has many restrictions).
     * ─────────────────────────────────────────────────────────────────── */
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, TELEMETRY_SIGNAL);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);   /* block it → sigwaitinfo */

    while (g_running) {
        /* ── Block here until the POSIX timer fires (every 500 ms) ───────
         * sigwaitinfo() atomically removes the signal from the pending set
         * and returns its siginfo_t.  If g_running becomes 0 and no signal
         * arrives, the loop still terminates at the next timer tick.
         * ──────────────────────────────────────────────────────────────── */
        siginfo_t info;
        int sig = sigwaitinfo(&sigset, &info);
        if (sig < 0) {
            if (errno == EINTR) continue;   /* interrupted by another signal */
            break;
        }

        if (!g_running) break;

        /* ── Case C: acquire shared mutex BEFORE heavy work ────────────── *
         * While this lock is held, the high-priority stability thread      *
         * that also needs it will be BLOCKED.  That is priority inversion: *
         * a CPU-intensive, low-priority task holds a resource that a       *
         * high-priority task urgently needs.                                *
         * ──────────────────────────────────────────────────────────────── */
        if (ta->test_case == CASE_C_PRIOINV) {
            pthread_mutex_lock(&g_shared_mutex);
            printf("[PRIOINV] Telemetry acquired mutex → Stability thread "
                   "is now BLOCKED (priority inversion occurring!)\n");
        }

        /* ── Simulate telemetry data transmission (log to screen) ─────── */
        (void)heavy_work();
        ta->counter++;

        printf("[TELEMETRY] Downlink packet #%ld sent at 500ms tick\n",
               ta->counter);

        if (ta->test_case == CASE_C_PRIOINV) {
            pthread_mutex_unlock(&g_shared_mutex);
            printf("[PRIOINV] Telemetry released mutex → Stability thread "
                   "can proceed.\n");
        }
    }

    printf("[THREAD] %s finished. Iterations: %ld\n", ta->name, ta->counter);
    return NULL;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  HELPER: create_telemetry_timer()
 *
 *  Creates a POSIX interval timer (timer_create) that fires every
 *  TELEMETRY_PERIOD_MS milliseconds.
 *
 *  The signal is directed to a specific thread (g_telemetry_tid) using
 *  SIGEV_THREAD_ID (Linux extension).  This ensures only the telemetry
 *  thread receives the signal, not the whole process.
 *
 *  Must be called AFTER g_telemetry_tid has been set (after thread create).
 * ══════════════════════════════════════════════════════════════════════════ */
static void create_telemetry_timer(void)
{
    struct sigevent sev;
    memset(&sev, 0, sizeof(sev));

    /*
     * SIGEV_THREAD_ID: send the signal to the specific thread identified
     * by _tid.  This is a Linux-specific extension (not strictly POSIX).
     * The alternative is SIGEV_SIGNAL (delivers to any thread in the
     * process that doesn't block it), which is less deterministic.
     */
    sev.sigev_notify            = SIGEV_THREAD_ID;
    sev.sigev_signo             = TELEMETRY_SIGNAL;
    sev.sigev_value.sival_int   = 0;
    /* _tid field: Linux-specific member inside the sigevent union          */
    sev._sigev_un._tid          = (pid_t)g_telemetry_tid;

    /*
     * Create the timer using the monotonic clock.
     * CLOCK_MONOTONIC is immune to system-time adjustments (NTP, etc.),
     * making it the right choice for periodic real-time tasks.
     */
    if (timer_create(CLOCK_MONOTONIC, &sev, &g_telemetry_timer) != 0) {
        perror("timer_create");
        exit(EXIT_FAILURE);
    }

    /* ── Set timer period and initial expiration ─────────────────────────
     * itimerspec.it_value    : delay before first expiration
     * itimerspec.it_interval : period for subsequent expirations
     * Both are set to TELEMETRY_PERIOD_MS for immediate start.
     * ─────────────────────────────────────────────────────────────────── */
    struct itimerspec its;
    its.it_value.tv_sec     = TELEMETRY_PERIOD_MS / 1000;
    its.it_value.tv_nsec    = (TELEMETRY_PERIOD_MS % 1000) * 1000000L;
    its.it_interval.tv_sec  = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    if (timer_settime(g_telemetry_timer, 0, &its, NULL) != 0) {
        perror("timer_settime");
        exit(EXIT_FAILURE);
    }

    printf("[TIMER]  POSIX interval timer created: period = %d ms  "
           "signal = SIGRTMIN (%d)\n",
           TELEMETRY_PERIOD_MS, TELEMETRY_SIGNAL);
}

/* ══════════════════════════════════════════════════════════════════════════
 *  HELPER: print_results()
 *
 *  Prints a comparative table of iterations completed by each thread
 *  during the EXPERIMENT_DURATION_SEC window.
 * ══════════════════════════════════════════════════════════════════════════ */
static void print_results(TestCase tc)
{
    const char *case_names[] = {
        "A - Normal    (SCHED_OTHER)",
        "B - Real-Time (SCHED_FIFO)",
        "C - Priority Inversion Demo"
    };

    long stab = args_stability.counter;
    long nav  = args_navigation.counter;
    long tel  = args_telemetry.counter;
    long total = stab + nav + tel;

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║         RESULTS — Test Case %s          \n", case_names[tc]);
    printf("║         Measurement window: %d seconds                       \n",
           EXPERIMENT_DURATION_SEC);
    printf("╠══════════════════════════════╦══════════╦══════════╦════════╣\n");
    printf("║ Thread                       ║ Priority ║  Iters   ║  %%CPU  ║\n");
    printf("╠══════════════════════════════╬══════════╬══════════╬════════╣\n");
    printf("║ %-28s ║   %3d    ║ %8ld ║ %5.1f%% ║\n",
           "Stability Control (CRITICAL)",
           args_stability.priority, stab,
           total > 0 ? 100.0 * stab / total : 0.0);
    printf("║ %-28s ║   %3d    ║ %8ld ║ %5.1f%% ║\n",
           "Navigation (MEDIUM)",
           args_navigation.priority, nav,
           total > 0 ? 100.0 * nav / total : 0.0);
    printf("║ %-28s ║   %3d    ║ %8ld ║ %5.1f%% ║\n",
           "Telemetry (LOW)",
           args_telemetry.priority, tel,
           total > 0 ? 100.0 * tel / total : 0.0);
    printf("╠══════════════════════════════╩══════════╬══════════╬════════╣\n");
    printf("║ TOTAL                                   ║ %8ld ║ 100.0%% ║\n",
           total);
    printf("╚═════════════════════════════════════════╩══════════╩════════╝\n");

    /* ── Analysis notes ──────────────────────────────────────────────── */
    printf("\n[ANALYSIS]\n");
    switch (tc) {
        case CASE_A_NORMAL:
            printf("  Case A (SCHED_OTHER): The CFS (Completely Fair Scheduler)\n"
                   "  distributes CPU time roughly equally among threads.\n"
                   "  Expect similar iteration counts across all three tasks.\n");
            break;
        case CASE_B_REALTIME:
            printf("  Case B (SCHED_FIFO): The stability thread (prio 80)\n"
                   "  dominates CPU time. Under SCHED_FIFO a higher-priority\n"
                   "  runnable thread pre-empts all lower-priority threads.\n"
                   "  Navigation runs only when stability yields; telemetry\n"
                   "  runs only on timer ticks (sigwaitinfo blocks it).\n");
            break;
        case CASE_C_PRIOINV:
            printf("  Case C (Priority Inversion): The stability thread (prio 80)\n"
                   "  is occasionally blocked waiting for the mutex held by\n"
                   "  telemetry (prio 10). During that window the navigation\n"
                   "  thread (prio 40) may run, effectively running at a\n"
                   "  priority higher than stability — the inversion.\n"
                   "  Fix: use a priority-inheritance mutex (PTHREAD_PRIO_INHERIT).\n");
            break;
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 *  HELPER: block_signal_globally()
 *
 *  Blocks TELEMETRY_SIGNAL in the calling thread (main and subsequently
 *  all threads spawned from it that don't override their signal mask).
 *  The telemetry thread later calls SIG_BLOCK again (belt-and-suspenders)
 *  and uses sigwaitinfo() to consume signals synchronously.
 *
 *  This prevents the signal from unexpectedly interrupting other threads.
 * ══════════════════════════════════════════════════════════════════════════ */
static void block_signal_globally(void)
{
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, TELEMETRY_SIGNAL);
    /* SIG_BLOCK: add to the current thread's blocked-signal mask           */
    if (pthread_sigmask(SIG_BLOCK, &sigset, NULL) != 0) {
        perror("pthread_sigmask");
        exit(EXIT_FAILURE);
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 *  MAIN
 * ══════════════════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[])
{
    /* ── Parse command-line argument ────────────────────────────────────── */
    if (argc != 2) {
        fprintf(stderr,
                "Usage: %s <case>\n"
                "  A  →  Case A: Normal      (SCHED_OTHER, no sudo needed)\n"
                "  B  →  Case B: Real-Time   (SCHED_FIFO,  requires sudo)\n"
                "  C  →  Case C: Prio Inver. (SCHED_FIFO,  requires sudo)\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    TestCase test_case;
    if      (strcmp(argv[1], "A") == 0) test_case = CASE_A_NORMAL;
    else if (strcmp(argv[1], "B") == 0) test_case = CASE_B_REALTIME;
    else if (strcmp(argv[1], "C") == 0) test_case = CASE_C_PRIOINV;
    else {
        fprintf(stderr, "Unknown case '%s'. Use A, B, or C.\n", argv[1]);
        return EXIT_FAILURE;
    }

    /* ── Select policy ──────────────────────────────────────────────────── */
    int policy = (test_case == CASE_A_NORMAL) ? SCHED_OTHER : SCHED_FIFO;

    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║       Flight Control System Simulator — TP Unidad 2         ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  Test case  : %s                                       \n",
           test_case == CASE_A_NORMAL   ? "A — Normal (SCHED_OTHER)     " :
           test_case == CASE_B_REALTIME ? "B — Real-Time (SCHED_FIFO)   " :
                                          "C — Priority Inversion Demo  ");
    printf("║  Duration   : %d seconds                                     \n",
           EXPERIMENT_DURATION_SEC);
    printf("║  Timer      : POSIX interval timer @ %d ms (telemetry)    \n",
           TELEMETRY_PERIOD_MS);
    printf("╚══════════════════════════════════════════════════════════════╝\n\n");

    /* ── Block TELEMETRY_SIGNAL in main (inherited by all child threads) ── */
    block_signal_globally();

    /* ── Initialize thread argument structs ─────────────────────────────── */
    memset(&args_stability,  0, sizeof(ThreadArgs));
    memset(&args_navigation, 0, sizeof(ThreadArgs));
    memset(&args_telemetry,  0, sizeof(ThreadArgs));

    args_stability.name      = "Stability-Control";
    args_stability.priority  = (policy == SCHED_FIFO) ? PRIO_STABILITY  : 0;
    args_stability.policy    = policy;
    args_stability.test_case = test_case;

    args_navigation.name     = "Navigation";
    args_navigation.priority = (policy == SCHED_FIFO) ? PRIO_NAVIGATION : 0;
    args_navigation.policy   = policy;
    args_navigation.test_case = test_case;

    args_telemetry.name      = "Telemetry";
    args_telemetry.priority  = (policy == SCHED_FIFO) ? PRIO_TELEMETRY  : 0;
    args_telemetry.policy    = policy;
    args_telemetry.test_case = test_case;

    /* ── Thread handles ─────────────────────────────────────────────────── */
    pthread_t tid_stability, tid_navigation, tid_telemetry;

    /* ── Launch threads ─────────────────────────────────────────────────── *
     * We use default attributes (NULL) and apply scheduling inside each    *
     * thread via pthread_setschedparam().  An alternative is to pre-set    *
     * the attributes with pthread_attr_setschedpolicy() before creation,   *
     * which also requires root.  Both approaches are equivalent.           *
     * ─────────────────────────────────────────────────────────────────── */

    printf("[MAIN]   Launching threads...\n");

    if (pthread_create(&tid_stability, NULL,
                       stability_thread,  &args_stability) != 0) {
        perror("pthread_create stability");
        return EXIT_FAILURE;
    }

    if (pthread_create(&tid_navigation, NULL,
                       navigation_thread, &args_navigation) != 0) {
        perror("pthread_create navigation");
        return EXIT_FAILURE;
    }

    if (pthread_create(&tid_telemetry, NULL,
                       telemetry_thread,  &args_telemetry) != 0) {
        perror("pthread_create telemetry");
        return EXIT_FAILURE;
    }

    /*
     * Save the telemetry thread's ID so create_telemetry_timer() can
     * direct signals to it via SIGEV_THREAD_ID.
     *
     * NOTE: pthread_t is the POSIX thread ID.  SIGEV_THREAD_ID uses the
     * kernel (OS-level) thread ID (gettid()).  We use (pid_t)tid_telemetry
     * which works on Linux where the pthread_t maps to the kernel tid on
     * the glibc/NPTL implementation.  For absolute portability you would
     * call gettid() from within the thread itself.
     */
    g_telemetry_tid = tid_telemetry;

    /*
     * Small delay to ensure the telemetry thread has started and blocked
     * its signal mask before the timer starts firing.
     */
    struct timespec short_wait = {0, 50000000L};  /* 50 ms */
    nanosleep(&short_wait, NULL);

    /* ── Create and arm the POSIX telemetry timer ────────────────────────  */
    create_telemetry_timer();

    /* ── Run experiment for EXPERIMENT_DURATION_SEC seconds ─────────────── */
    printf("[MAIN]   Experiment running for %d seconds...\n\n",
           EXPERIMENT_DURATION_SEC);
    sleep(EXPERIMENT_DURATION_SEC);

    /* ── Stop all threads ────────────────────────────────────────────────── *
     * Setting g_running = 0 causes each thread's while-loop to exit after  *
     * its current iteration.  We then join to ensure clean termination.    *
     * ─────────────────────────────────────────────────────────────────── */
    printf("\n[MAIN]   Time is up — stopping threads...\n");
    g_running = 0;

    /* Disarm the timer (set it to zero to prevent further signals)         */
    struct itimerspec disarm = {{0,0},{0,0}};
    timer_settime(g_telemetry_timer, 0, &disarm, NULL);

    /*
     * Send one final TELEMETRY_SIGNAL so the telemetry thread, which is
     * blocking in sigwaitinfo(), wakes up and sees g_running == 0.
     * Without this it would wait indefinitely for the next (now-cancelled)
     * timer tick.
     */
    pthread_kill(tid_telemetry, TELEMETRY_SIGNAL);

    /* ── Join all threads ────────────────────────────────────────────────── */
    pthread_join(tid_stability, NULL);
    pthread_join(tid_navigation, NULL);
    pthread_join(tid_telemetry,  NULL);

    /* ── Clean up POSIX timer and mutex ─────────────────────────────────── */
    timer_delete(g_telemetry_timer);
    pthread_mutex_destroy(&g_shared_mutex);

    /* ── Print comparative results table ─────────────────────────────────  */
    print_results(test_case);

    return EXIT_SUCCESS;
}

/*
 * ============================================================================
 * END OF FILE
 * ============================================================================
 *
 * EXPECTED BEHAVIOUR SUMMARY
 * ──────────────────────────
 * Case A (SCHED_OTHER / CFS):
 *   All three threads get roughly equal CPU time.  The scheduler uses
 *   virtual runtime (vruntime) to keep fairness.  Iteration counts will
 *   be approximately equal.
 *
 * Case B (SCHED_FIFO, priorities 80/40/10):
 *   Under SCHED_FIFO, the highest-priority RUNNABLE thread runs until it
 *   either blocks or is pre-empted by an even higher-priority thread.
 *   Since stability (80) is always runnable, it monopolises the CPU.
 *   Navigation (40) barely runs.  Telemetry runs only on timer signal.
 *   → Stability will have vastly more iterations than the others.
 *
 * Case C (Priority Inversion):
 *   Telemetry (prio 10) grabs the mutex, does heavy_work(), releases it.
 *   Stability (prio 80) blocks on the same mutex while telemetry holds it.
 *   Navigation (prio 40) — which does NOT need the mutex — can run during
 *   that window, effectively achieving a higher effective priority than the
 *   stability task.  This is priority inversion.
 *   Real-time systems prevent this with priority-inheritance mutexes:
 *     pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
 *   With priority inheritance the OS temporarily boosts the telemetry
 *   thread's priority to 80 while it holds the mutex, so navigation (40)
 *   cannot pre-empt it.
 * ============================================================================
 */