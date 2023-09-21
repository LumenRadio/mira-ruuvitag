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

#include "gpiote-nrf-drv.h"

uint8_t gpiote_edge_event_init(
    nrfx_gpiote_pin_t te_pin,
    nrf_gpiote_polarity_t polarity,
    nrf_gpio_pin_pull_t pull,
    nrfx_gpiote_evt_handler_t callback)
{
    uint8_t status = 0;
    nrfx_gpiote_in_config_t te_config;
    if (!nrfx_gpiote_is_init()) {
        nrfx_gpiote_init();
    }
    te_config.hi_accuracy = false;
    te_config.is_watcher = false;
    te_config.pull = pull;
    te_config.sense = polarity;

    if (nrfx_gpiote_in_init(te_pin, &te_config, callback) == 0) {
        status = 1;
        nrfx_gpiote_in_event_enable(te_pin, true);
    }
    return status;
}