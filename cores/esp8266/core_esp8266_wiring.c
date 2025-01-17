/* 
 core_esp8266_wiring.c - implementation of Wiring API for esp8266

 Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "wiring_private.h"
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "cont.h"

extern void esp_schedule();
extern void esp_yield();

#ifndef ESP8266_NOWIFI
static os_timer_t delay_timer;
static os_timer_t micros_overflow_timer;
#endif
static uint32_t micros_at_last_overflow_tick = 0;
static uint32_t micros_overflow_count = 0;
#define ONCE 0
#define REPEAT 1

void delay_end(void* arg) {
    (void) arg;
    esp_schedule();
}

void delay(unsigned long ms) {
#ifndef ESP8266_NOWIFI
    if(ms) {
        os_timer_setfn(&delay_timer, (os_timer_func_t*) &delay_end, 0);
        os_timer_arm(&delay_timer, ms, ONCE);
    } else {
        esp_schedule();
    }
    esp_yield();
    if(ms) {
        os_timer_disarm(&delay_timer);
    }
#else
    os_delay_us(ms * 1000);
#endif
}

#ifndef ESP8266_NOWIFI
void micros_overflow_tick(void* arg) {
    (void) arg;
    uint32_t m = system_get_time();
    if(m < micros_at_last_overflow_tick)
        ++micros_overflow_count;
    micros_at_last_overflow_tick = m;
}
#endif

unsigned long ICACHE_RAM_ATTR millis() {
    uint32_t m = system_get_time();

#ifdef ESP8266_NOWIFI
    if (m < micros_at_last_overflow_tick)
        ++micros_overflow_count;
    micros_at_last_overflow_tick = m;

    return micros_overflow_count * 4294967 + m / 1000;
#else
    uint32_t c = micros_overflow_count + ((m < micros_at_last_overflow_tick) ? 1 : 0);
    return c * 4294967 + m / 1000;
#endif
}

unsigned long ICACHE_RAM_ATTR micros() {
    return system_get_time();
}

void ICACHE_RAM_ATTR delayMicroseconds(unsigned int us) {
    os_delay_us(us);
}

void init() {
    initPins();
    timer1_isr_init();
#ifndef ESP8266_NOWIFI
    os_timer_setfn(&micros_overflow_timer, (os_timer_func_t*) &micros_overflow_tick, 0);
    os_timer_arm(&micros_overflow_timer, 60000, REPEAT);
#endif
}
