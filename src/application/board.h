/*
 * Copyright (c) 2018, LumenRadio AB All rights reserved.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BOARD_H
#define BOARD_H

#include <mira.h>

#define board_selection 0

/*
 * 0 = Ruuvi-tag
 * 1 = nRF52840DK
 * 2 = Ruuvi devboard on nRF52832
 * 3 = nRF52832DK
 */

/* DEBUG FOR RTT */
#define RTT_DEBUG 0
/* ------------- */

#if RTT_DEBUG
#define BOARD_STDOUT_RTT_ID 0
#endif

#if (board_selection == 0)
#include "board-ruuvi.h"

#elif (board_selection == 1)
#include "board-nrf52840dk.h"

#elif (board_selection == 2)
#include "board-click-bme280.h"

#elif (board_selection == 3)
#include "board-nrf52832dk.h"

#endif

void board_setup(void);

void board_led_set(int num, int val);

#endif
