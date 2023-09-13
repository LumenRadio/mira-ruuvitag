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
 *
 *
 */

#ifndef _DPS310REG_H_
#define _DPS310REG_H_

/* Register addresses */
#define DPS310_REVISION_ID_REG            (0x0D)
#define DPS310_ID                         (0x10)
#define PRS_B2                            (0x00)
#define PRS_B1                            (0x01)
#define PRS_B0                            (0x02)
#define TMP_B2                            (0x03)
#define TMP_B1                            (0x04)
#define TMP_B0                            (0x05)
#define PRS_CFG                           (0x06)
#define TMP_CFG                           (0x07)
#define MEAS_CFG                          (0x08)
#define COEF_SRCE                         (0x28)
#define CAL_C0                            (0x10)

/* Operating mode commands */
#define IDLE                              (0x00)
#define CMD_PRS                           (0x01) // read pressure
#define CMD_TMP                           (0x02) // read temperature

/* Measurement rates and oversampling rates*/
#define PM_RATE_8                         (0x30)
#define PM_PRC_8                          (0x03)

#define TMP_RATE_8                        (0x30)
#define TMP_PRC_8                         (0x03)
#define TMP_EXT                           1 << 7

/* SPI commands */
#define DPS310_READ                          (0x80)
#define DPS310_WRITE                         (0x00)

#endif