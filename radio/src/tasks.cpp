/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"

RTOS_TASK_HANDLE menusTaskId;
RTOS_DEFINE_STACK(menusStack, MENUS_STACK_SIZE);

RTOS_TASK_HANDLE mixerTaskId;
RTOS_DEFINE_STACK(mixerStack, MIXER_STACK_SIZE);

RTOS_TASK_HANDLE audioTaskId;
RTOS_DEFINE_STACK(audioStack, AUDIO_STACK_SIZE);

#if defined(PCBTANGO)
RTOS_TASK_HANDLE crossfireTaskId;
RTOS_DEFINE_STACK(crossfireStack, CROSSFIRE_STACK_SIZE);

RTOS_TASK_HANDLE systemTaskId;
RTOS_DEFINE_STACK(systemStack, SYSTEM_STACK_SIZE);
#endif

RTOS_MUTEX_HANDLE audioMutex;
RTOS_MUTEX_HANDLE mixerMutex;

enum TaskIndex {
  MENU_TASK_INDEX,
  MIXER_TASK_INDEX,
  AUDIO_TASK_INDEX,
  CLI_TASK_INDEX,
  BLUETOOTH_TASK_INDEX,
  TASK_INDEX_COUNT,
  MAIN_TASK_INDEX = 255
};

void stackPaint()
{
  menusStack.paint();
  mixerStack.paint();
  audioStack.paint();
#if defined(CLI)
  cliStack.paint();
#endif
}

volatile uint16_t timeForcePowerOffPressed = 0;

bool isForcePowerOffRequested()
{
  if (pwrOffPressed()) {
    if (timeForcePowerOffPressed == 0) {
      timeForcePowerOffPressed = get_tmr10ms();
    }
    else {
      uint16_t delay = (uint16_t)get_tmr10ms() - timeForcePowerOffPressed;
      if (delay > 1000/*10s*/) {
        return true;
      }
    }
  }
  else {
    resetForcePowerOffRequest();
  }
  return false;
}

bool isModuleSynchronous(uint8_t module)
{
  uint8_t protocol = moduleState[module].protocol;
  if (protocol == PROTOCOL_CHANNELS_PXX2 || protocol == PROTOCOL_CHANNELS_CROSSFIRE || protocol == PROTOCOL_CHANNELS_NONE)
    return true;
#if defined(INTMODULE_USART) || defined(EXTMODULE_USART)
  if (protocol == PROTOCOL_CHANNELS_PXX1_SERIAL)
    return true;
#elif defined(PCBTANGO)
    return true;
#endif
  return false;
}

void sendSynchronousPulses()
{
  for (uint8_t module = 0; module < NUM_MODULES; module++) {
    if (isModuleSynchronous(module) && setupPulses(module)) {
#if defined(HARDWARE_INTERNAL_MODULE)
      if (module == INTERNAL_MODULE)
        intmoduleSendNextFrame();
#endif
      if (module == EXTERNAL_MODULE)
        extmoduleSendNextFrame();
    }
  }
}

uint32_t nextMixerTime[NUM_MODULES];

TASK_FUNCTION(mixerTask)
{
  static uint32_t lastRunTime;
  s_pulses_paused = true;

  while (1) {
#if defined(PCBX9D) || defined(PCBX7)
    // SBUS on Hearbeat PIN (which is a serial RX)
    processSbusInput();
#endif

#if defined(GYRO)
    gyro.wakeup();
#endif

#if defined(BLUETOOTH)
    bluetooth.wakeup();
#endif

    RTOS_WAIT_TICKS(1);

#if defined(SIMU)
    if (pwrCheck() == e_power_off) {
      TASK_RETURN();
    }
#else
    if (isForcePowerOffRequested()) {
      pwrOff();
    }
#endif

    uint32_t now = RTOS_GET_MS();
    bool run = false;
#if defined(PCBTANGO) && !defined(SIMU)
    if (isMixerTaskScheduled()) {
      clearMixerTaskSchedule();
      run = true;
    }
#endif
    if ((now - lastRunTime) >= 10) {     // run at least every 10ms
      run = true;
    }
    else if (now == nextMixerTime[0]) {
      run = true;
    }
#if NUM_MODULES >= 2
    else if (now == nextMixerTime[1]) {
      run = true;
    }
#endif
    if (!run) {
      continue;  // go back to sleep
    }

    lastRunTime = now;

    if (!s_pulses_paused) {
      uint16_t t0 = getTmr2MHz();

      DEBUG_TIMER_START(debugTimerMixer);
      RTOS_LOCK_MUTEX(mixerMutex);
      doMixerCalculations();
      DEBUG_TIMER_START(debugTimerMixerCalcToUsage);
      DEBUG_TIMER_SAMPLE(debugTimerMixerIterval);
      RTOS_UNLOCK_MUTEX(mixerMutex);
      DEBUG_TIMER_STOP(debugTimerMixer);

#if defined(STM32) && !defined(SIMU)
      if (getSelectedUsbMode() == USB_JOYSTICK_MODE) {
        usbJoystickUpdate();
      }
  #if defined(PCBTANGO)
      tangoUpdateChannel();
  #endif
#endif

#if defined(TELEMETRY_FRSKY) || defined(PCBTANGO)
      DEBUG_TIMER_START(debugTimerTelemetryWakeup);
      telemetryWakeup();
      DEBUG_TIMER_STOP(debugTimerTelemetryWakeup);
#endif

      if (heartbeat == HEART_WDT_CHECK) {
        wdt_reset();
        heartbeat = 0;
      }

      t0 = getTmr2MHz() - t0;
      if (t0 > maxMixerDuration) maxMixerDuration = t0;

      sendSynchronousPulses();
    }
  }
}

void scheduleNextMixerCalculation(uint8_t module, uint16_t period_ms)
{
  // Schedule next mixer calculation time,

  if (isModuleSynchronous(module)) {
    nextMixerTime[module] += period_ms / RTOS_MS_PER_TICK;
    if (nextMixerTime[module] < RTOS_GET_TIME()) {
      // we are late ... let's add some small delay
      nextMixerTime[module] = (uint32_t) RTOS_GET_TIME() + (period_ms / RTOS_MS_PER_TICK);
    }
  }
  else {
    // for now assume mixer calculation takes 2 ms.
    nextMixerTime[module] = (uint32_t) RTOS_GET_TIME() + (period_ms / RTOS_MS_PER_TICK) - 1 /* 1 tick in advance*/;
  }

  DEBUG_TIMER_STOP(debugTimerMixerCalcToUsage);
}

#define MENU_TASK_PERIOD_TICKS         (50 / RTOS_MS_PER_TICK)    // 50ms

#if defined(COLORLCD) && defined(CLI)
bool perMainEnabled = true;
#endif

TASK_FUNCTION(menusTask)
{
  opentxInit();

#if defined(PWR_BUTTON_PRESS)
  while (1) {
    uint32_t pwr_check = pwrCheck();
    if (pwr_check == e_power_off) {
      break;
    }
    else if (pwr_check == e_power_press) {
      RTOS_WAIT_TICKS(MENU_TASK_PERIOD_TICKS);
      continue;
    }
#else
  while (pwrCheck() != e_power_off) {
#endif
    uint32_t start = (uint32_t)RTOS_GET_TIME();
    DEBUG_TIMER_START(debugTimerPerMain);
#if defined(COLORLCD) && defined(CLI)
    if (perMainEnabled) {
      perMain();
    }
#else
    perMain();
#endif
    DEBUG_TIMER_STOP(debugTimerPerMain);
    // TODO remove completely massstorage from sky9x firmware
    uint32_t runtime = ((uint32_t)RTOS_GET_TIME() - start);
    // deduct the thread run-time from the wait, if run-time was more than
    // desired period, then skip the wait all together
    if (runtime < MENU_TASK_PERIOD_TICKS) {
      RTOS_WAIT_TICKS(MENU_TASK_PERIOD_TICKS - runtime);
    }

    resetForcePowerOffRequest();
  }

#if defined(PCBTANGO) && defined(LIBCRSF_ENABLE_OPENTX_RELATED) && defined(LIBCRSF_ENABLE_SD)
    if((*(uint32_t *)CROSSFIRE_TASK_ADDRESS != 0xFFFFFFFF) &&
        getSelectedUsbMode() != USB_MASS_STORAGE_MODE && sdMounted()){
        set_crsf_flag( CRSF_FLAG_EEPROM_SAVE );
        while( get_crsf_flag( CRSF_FLAG_EEPROM_SAVE )){
        RTOS_WAIT_TICKS(1);
      }
    }
#endif

#if defined(PCBX9E)
  toplcdOff();
#endif

#if defined(PCBHORUS)
  ledOff();
#endif

  drawSleepBitmap();
  opentxClose();
#if defined(PCBTANGO)
  if (IS_CHARGING_STATE() && !IS_CHARGING_FAULT() && usbPlugged()) {
    NVIC_SystemReset();
  }
#endif
  boardOff(); // Only turn power off if necessary

  TASK_RETURN();
}

#if defined(PCBTANGO) && !defined(SIMU)
TASK_FUNCTION(systemTask)
{
  static uint32_t get_modelid_delay = 0;
  set_model_id_needed = true;

  while(1) {
    if( get_crsf_flag( CRSF_FLAG_SHOW_BOOTLOADER_ICON )){
      static uint32_t delayCount = 0;
      if(delayCount == 0){
        delayCount = RTOS_GET_TIME();
        RTOS_DEL_TASK(menusTaskId);
        lcdOn();
        drawDownload();
      }
      if(RTOS_GET_TIME() - delayCount > 100){
        NVIC_SystemReset();
      }
    }

    crsfSharedFifoHandler();
    crsfEspHandler();
#if defined(AGENT) && !defined(SIMU)
    AgentHandler();
#endif
    if (set_model_id_needed && g_model.header.modelId[EXTERNAL_MODULE] != 0) {
      crsfSetModelID();
      set_model_id_needed = false;
      crsfGetModelID();
      get_modelid_delay = get_tmr10ms();
    }
    if (get_modelid_delay && (get_tmr10ms() - get_modelid_delay) > 100) {
      if (current_crsf_model_id == g_model.header.modelId[EXTERNAL_MODULE]) {
        /* Set model id successfully */
        TRACE("Set model id for crossfire success, current id = %d\r\n", current_crsf_model_id);
      }
      else {
        /* Set model id failed */
        TRACE("Set model id for crossfire failed, current id = %d\r\n", current_crsf_model_id);
        /* do something else here? */
      }
      get_modelid_delay = 0;
    }
  }
  TASK_RETURN();
}
#endif

void tasksStart()
{
  RTOS_INIT();

#if defined(CLI)
  cliStart();
#endif

  RTOS_CREATE_TASK(mixerTaskId, mixerTask, "Mixer", mixerStack, MIXER_STACK_SIZE, MIXER_TASK_PRIO);
  RTOS_CREATE_TASK(menusTaskId, menusTask, "Menus", menusStack, MENUS_STACK_SIZE, MENUS_TASK_PRIO);
#if (defined(PCBTANGO)) && defined(CROSSFIRE_TASK) && !defined(SIMU)
  uint8_t taskFlag[TASK_FLAG_MAX] = {0};
  // Test if crossfire task is available and start it
  if (*(uint32_t *)CROSSFIRE_TASK_ADDRESS != 0xFFFFFFFF ) {
    RTOS_CREATE_TASK(crossfireTaskId, (FUNCPtr)CROSSFIRE_TASK_ADDRESS, "crossfire", crossfireStack, CROSSFIRE_STACK_SIZE, CROSSFIRE_TASK_PRIORITY);
    RTOS_CREATE_FLAG( taskFlag[XF_TASK_FLAG] );

    RTOS_CREATE_TASK(systemTaskId, systemTask, "system", systemStack, SYSTEM_STACK_SIZE, RTOS_SYS_TASK_PRIORITY);
    RTOS_CREATE_FLAG( taskFlag[CRSF_SD_TASK_FLAG] );
    RTOS_CREATE_FLAG( taskFlag[BOOTLOADER_ICON_WAIT_FLAG] );

    for( uint8_t i = 0; i < TASK_FLAG_MAX; i++ ){
      crossfireSharedData.taskFlag[i] = taskFlag[i];
    }
  }
#endif

#if !defined(SIMU)
  RTOS_CREATE_TASK(audioTaskId, audioTask, "Audio", audioStack, AUDIO_STACK_SIZE, AUDIO_TASK_PRIO);
#endif

  RTOS_CREATE_MUTEX(audioMutex);
  RTOS_CREATE_MUTEX(mixerMutex);

  RTOS_START();
}
