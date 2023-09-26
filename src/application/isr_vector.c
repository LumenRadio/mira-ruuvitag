/*----------------------------------------------------------------------------
Copyright (c) 2020 LumenRadio AB
This code is the property of Lumenradio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.
----------------------------------------------------------------------------*/
#include "mira_nrf.h"

extern unsigned long __StackTop[];
extern void mira_Reset_Handler(
    void);

void Reset_Handler(
    void)
{
    mira_nrf_clk_config_t config;
#ifdef MIRA_LFCLK_EXTERNAL_SOURCE_BYPASS_ENABLED
    config.lfclk_source = MIRA_NRF_LFCLK_EXTERNAL_SOURCE_BYPASS_ENABLED,
#elif defined(MIRA_LFCLK_EXTERNAL_SOURCE_BYPASS_DISABLED)
    config.lfclk_source = MIRA_NRF_LFCLK_EXTERNAL_SOURCE_BYPASS_DISABLED,
#else
    config.lfclk_source = MIRA_NRF_LFCLK_CRYSTAL,
#endif

    mira_nrf_clk_init(&config);
    mira_Reset_Handler();
}

typedef void (*ISR_vector)(
    void);

void Reset_Handler(
    void);
void NMI_Handler(
    void);
void HardFault_Handler(
    void);
void MemoryManagement_Handler(
    void);
void BusFault_Handler(
    void);
void UsageFault_Handler(
    void);
void SVC_Handler(
    void);
void DebugMon_Handler(
    void);
void PendSV_Handler(
    void);
void SysTick_Handler(
    void);

void POWER_CLOCK_IRQHandler(
    void);
void RADIO_IRQHandler(
    void);
void mira_UARTE0_UART0_IRQHandler(
    void);
void mira_SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler(
    void);
void SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler(
    void);
void mira_NFCT_IRQHandler(
    void);
void GPIOTE_IRQHandler(
    void);
void mira_SAADC_IRQHandler(
    void);
void TIMER0_IRQHandler(
    void);
void TIMER1_IRQHandler(
    void);
void TIMER2_IRQHandler(
    void);
void RTC0_IRQHandler(
    void);
void TEMP_IRQHandler(
    void);
void RNG_IRQHandler(
    void);
void ECB_IRQHandler(
    void);
void CCM_AAR_IRQHandler(
    void);
void mira_WDT_IRQHandler(
    void);
void RTC1_IRQHandler(
    void);
void QDEC_IRQHandler(
    void);
void COMP_LPCOMP_IRQHandler(
    void);
void mira_SWI0_EGU0_IRQHandler(
    void);
void mira_SWI1_EGU1_IRQHandler(
    void);
void SWI2_EGU2_IRQHandler(
    void);
void SWI3_EGU3_IRQHandler(
    void);
void SWI4_EGU4_IRQHandler(
    void);
void SWI5_EGU5_IRQHandler(
    void);
void TIMER3_IRQHandler(
    void);
void mira_TIMER4_IRQHandler(
    void);
void PWM0_IRQHandler(
    void);
void PDM_IRQHandler(
    void);
void MWU_IRQHandler(
    void);
void PWM1_IRQHandler(
    void);
void PWM2_IRQHandler(
    void);
void mira_SPIM2_SPIS2_SPI2_IRQHandler(
    void);
void mira_RTC2_IRQHandler(
    void);
void I2S_IRQHandler(
    void);
void FPU_IRQHandler(
    void);

const ISR_vector __isr_vector[128] __attribute__((section(".isr_vector"))) = {
    (ISR_vector) (unsigned long) __StackTop,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemoryManagement_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    (ISR_vector) 0,
    (ISR_vector) 0,
    (ISR_vector) 0,
    (ISR_vector) 0,
    SVC_Handler,
    DebugMon_Handler,
    (ISR_vector) 0,
    PendSV_Handler,
    SysTick_Handler,

    /* External Interrupts */
    POWER_CLOCK_IRQHandler,                       /*  0 */
    RADIO_IRQHandler,                             /*  1 */
    mira_UARTE0_UART0_IRQHandler,                      /*  2 */
    mira_SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler, /*  3 */
    SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler, /*  4 */
    mira_NFCT_IRQHandler,                              /*  5 */
    GPIOTE_IRQHandler,                       /*  6 */
    mira_SAADC_IRQHandler,                             /*  7 */
    TIMER0_IRQHandler,                            /*  8 */
    TIMER1_IRQHandler,                            /*  9 */
    TIMER2_IRQHandler,                            /* 10 */
    RTC0_IRQHandler,                              /* 11 */
    TEMP_IRQHandler,                              /* 12 */
    RNG_IRQHandler,                               /* 13 */
    ECB_IRQHandler,                               /* 14 */
    CCM_AAR_IRQHandler,                           /* 15 */
    mira_WDT_IRQHandler,                               /* 16 */
    RTC1_IRQHandler,                              /* 17 */
    QDEC_IRQHandler,                              /* 18 */
    COMP_LPCOMP_IRQHandler,                       /* 19 */
    mira_SWI0_EGU0_IRQHandler,                         /* 20 */
    mira_SWI1_EGU1_IRQHandler,                         /* 21 */
    SWI2_EGU2_IRQHandler,                         /* 22 */
    SWI3_EGU3_IRQHandler,                         /* 23 */
    SWI4_EGU4_IRQHandler,                         /* 24 */
    SWI5_EGU5_IRQHandler,                         /* 25 */
    TIMER3_IRQHandler,                            /* 26 */
    mira_TIMER4_IRQHandler,                            /* 27 */
    PWM0_IRQHandler,                              /* 28 */
    PDM_IRQHandler,                               /* 29 */
    (ISR_vector) 0,                               /* 30 */
    (ISR_vector) 0,                               /* 31 */
    MWU_IRQHandler,                               /* 32 */
    PWM1_IRQHandler,                              /* 33 */
    PWM2_IRQHandler,                              /* 34 */
    mira_SPIM2_SPIS2_SPI2_IRQHandler,                  /* 35 */
    mira_RTC2_IRQHandler,                              /* 36 */
    I2S_IRQHandler,                               /* 37 */
    FPU_IRQHandler                                /* 38 */
};

