#include "ch.h"
#include "hal.h"
#include "shell.h"
#include "evtimer.h"
#include "chprintf.h"

#include "ff.h"

#include "BlinkThreads.h"
#include "SampleAndControl.h"
#include "RearWheel.h"

/*===========================================================================*/
/* Card insertion monitor.                                                   */
/*===========================================================================*/

#define POLLING_INTERVAL                10
#define POLLING_DELAY                   10

/**
 * @brief   Card monitor timer.
 */
static VirtualTimer tmr;

/**
 * @brief   Debounce counter.
 */
static unsigned cnt;

/**
 * @brief   Card event sources.
 */
static EventSource inserted_event, removed_event;

/**
 * @brief   Insertion monitor timer callback function.
 *
 * @param[in] p         pointer to the @p BaseBlockDevice object
 *
 * @notapi
 */
static void tmrfunc(void *p) {
  BaseBlockDevice *bbdp = static_cast<BaseBlockDevice *>(p);

  chSysLockFromIsr();
  if (cnt > 0) {
    if (blkIsInserted(bbdp)) {
      if (--cnt == 0) {
        chEvtBroadcastI(&inserted_event);
      }
    }
    else
      cnt = POLLING_INTERVAL;
  }
  else {
    if (!blkIsInserted(bbdp)) {
      cnt = POLLING_INTERVAL;
      chEvtBroadcastI(&removed_event);
    }
  }
  chVTSetI(&tmr, MS2ST(POLLING_DELAY), tmrfunc, bbdp);
  chSysUnlockFromIsr();
}

/**
 * @brief   Polling monitor start.
 *
 * @param[in] p         pointer to an object implementing @p BaseBlockDevice
 *
 * @notapi
 */
static void tmr_init(void *p)
{
  chEvtInit(&inserted_event);
  chEvtInit(&removed_event);
  chSysLock();
  cnt = POLLING_INTERVAL;
  chVTSetI(&tmr, MS2ST(POLLING_DELAY), tmrfunc, p);
  chSysUnlock();
}

/*===========================================================================*/
/* FatFs related.                                                            */
/*===========================================================================*/

/**
 * @brief FS object.
 */
static FATFS SDC_FS;

/* FS mounted and ready.*/
static bool fs_ready = false;

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

static void cmd_threads(BaseSequentialStream *chp, int argc, __attribute__((unused)) char *argv[])
{
  static const char *states[] = {THD_STATE_NAMES};
  Thread *tp;

  if (argc > 0) {
    chprintf(chp, "Usage: threads\r\n");
    return;
  }
  chprintf(chp, "    addr    stack prio refs     state     time name\r\n");
  tp = chRegFirstThread();
  do {
    chprintf(chp, "%.8lx %.8lx %4lu %4lu %9s %8lu %s\r\n",
            (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
            (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
            states[tp->p_state], (uint32_t)tp->p_time,
            tp->p_name);
    tp = chRegNextThread(tp);
  } while (tp != NULL);
}

static const ShellCommand commands[] = {
  {"speed", RearWheel::shellcmd},     // disable/enable speed control, select set point
//  {"yawrate", YawRateController::shellcmd}, // disable/enable yawrate control, select set point
  {"control_loop", SampleAndControl::chshellcmd},
  {"threads", cmd_threads},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SD2,
  commands
};

/*===========================================================================*/
/* Main and generic code.                                                    */
/*===========================================================================*/

/*
 * MMC card insertion event.
 */
static void InsertHandler(__attribute__((unused)) eventid_t id)
{
  FRESULT err;

  /*
   * On insertion SDC initialization and FS mount.
   */
  if (sdcConnect(&SDCD1))
    return;

  err = f_mount(0, &SDC_FS);
  if (err != FR_OK) {
    sdcDisconnect(&SDCD1);
    return;
  }
  fs_ready = true;
}

/*
 * MMC card removal event.
 */
static void RemoveHandler(__attribute__((unused)) eventid_t id)
{
  sdcDisconnect(&SDCD1);
  fs_ready = false;
}

/*
 * Application entry point.
 */
int main()
{
  static const evhandler_t evhndl[] = { InsertHandler, RemoveHandler };
  Thread *shelltp = NULL;
  static struct EventListener el0, el1;

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();
  chRegSetThreadName("main");

  shellInit();            // Shell manager initialization

  sdStart(&SD2, NULL);    // Activate serial driver 2, default configuration

  sdcStart(&SDCD1, NULL); // Activate SDC driver 1, default configuration
  tmr_init(&SDCD1);       // Activates the card insertion monitor.

  // Blink threads
  static WORKING_AREA(waFileSystemBlinkThread, 128);
  chThdCreateStatic(waFileSystemBlinkThread, sizeof(waFileSystemBlinkThread),
                    NORMALPRIO, (tfunc_t) FileSystemBlinkThread, &fs_ready);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and listen for events.
   */
  chEvtRegister(&inserted_event, &el0, 0);
  chEvtRegister(&removed_event, &el1, 1);
  static WORKING_AREA(waShell, 2048);
  while (true) {
    if (!shelltp)
      shelltp = shellCreateStatic(&shell_cfg1, waShell, sizeof(waShell), NORMALPRIO);
    else if (chThdTerminated(shelltp)) {
      shelltp = NULL;           /* Triggers spawning of a new shell.        */
    }
    chEvtDispatch(evhndl, chEvtWaitOne(ALL_EVENTS));
  }
}
