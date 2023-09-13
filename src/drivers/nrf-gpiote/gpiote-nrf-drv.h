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

#ifndef GPIOTE_NRF_DRV_H
#define GPIOTE_NRF_DRV_H

#include "nrf_drv_gpiote.h"
#include "nrfx_gpiote.h"

typedef enum {
    NRF_EDGE_RISING,  /**< @brief A low to high transition */
    NRF_EDGE_FALLING, /**< @brief A high to low transition */
    NRF_EDGE_ANY      /**< @brief A low to high or a high to low transition */
} nrf_edge_t;

/* Enable edge events using the nRF SDk */
uint8_t gpiote_edge_event_init(
    nrfx_gpiote_pin_t te_pin,
    nrf_gpiote_polarity_t polarity,
    nrf_gpio_pin_pull_t pull,
    nrfx_gpiote_evt_handler_t callback);

#endif