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

void displayKeyState(uint8_t x, uint8_t y, uint8_t key)
{
  uint8_t t = keyState(key);
  lcdDrawChar(x, y, t+'0', t ? INVERS : 0);
}

void displaySwitchState(uint8_t x, uint8_t y, uint8_t sw)
{
  swsrc_t t = switchState(sw);
  lcdDrawChar(x, y, (t ? '1' : '0'), t ? INVERS : 0);
}

void menuRadioDiagKeys(event_t event)
{
  SIMPLE_SUBMENU(STR_MENU_RADIO_SWITCHES, 1);
  for (uint8_t i=0; i<9; i++) {
    coord_t y;
    if (i < TRM_BASE) {
      y = MENU_HEADER_HEIGHT + FH + FH*i;
      lcdDrawTextAtIndex(0, y, STR_VKEYS, (TRM_BASE-1-i), 0);
      displayKeyState(5*FW+2, y, KEY_MENU+(TRM_BASE-1-i));
    }

    if (i < NUM_SWITCHES) {
      if (SWITCH_EXISTS(i)) {
        getvalue_t val = getValue(MIXSRC_FIRST_SWITCH+i);
        getvalue_t sw = ((val < 0) ? 3*i+1 : ((val == 0) ? 3*i+2 : 3*i+3));
        drawSwitch(8*FW+30, y+10, sw, 0);
      }
    }
  }

#if defined(ROTARY_ENCODER_NAVIGATION)
  coord_t y = MENU_HEADER_HEIGHT;
  coord_t x = 19*FW;
  lcdDrawTextAtIndex(14*FW, y, STR_VRENCODERS, 0, 0);
  lcdDrawNumber(x, y, rotencValue, LEFT);
#endif

}