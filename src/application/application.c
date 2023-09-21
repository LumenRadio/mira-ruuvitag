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

#include <mira.h>
#include <stdio.h>
#include <string.h>

#include "application.h"

#include "app-config.h"
#include "net-status.h"
#include "sensors.h"
// #include "net-fota.h"

PROCESS(main_proc, "Main process");

PROCESS_THREAD(main_proc, ev, data) {
    static mira_net_config_t netconf;
    mira_status_t res;
    PROCESS_BEGIN();

    printf("Main process started\n");

    memset(&netconf, 0, sizeof(mira_net_config_t));
    netconf.pan_id = app_config.net_panid;
    memcpy(netconf.key, app_config.net_key, 16);
    netconf.mode = MIRA_NET_MODE_MESH;
    netconf.rate = app_config.net_rate;


    res = mira_net_init(&netconf);

    if (res != MIRA_SUCCESS){
        printf("!mira_net_init returned: %d\n", res);
    }
    net_status_init();

    sensors_init();

    PROCESS_END();
}
