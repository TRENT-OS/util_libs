/*
 * Copyright (C) 2022, HENSOLDT Cyber GmbH
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <stdint.h>

/* The Nvidia Tegra X1 TRM describes the individual CAR registers in chapter 5.2. */

// Contains the absolute values for bit fields within the registers defined in clk_ctrl_register.
enum clk_id {
    /* 0 */
    /* 1 */
    /* 2 */
    CLK_ENB_ISPB = 3, // Enable clock - ISPB
    CLK_ENB_RTC = 4, // Enable clock to RTC controller - enabled on reset
    CLK_ENB_TMR = 5, // Enable clock to Timer controller (sticky) - enabled on reset
    CLK_ENB_UARTA = 6, // Enable clock to UARTA controller
    CLK_ENB_UARTB = 7, // Enable clock to UARTB/VFIR controller
    CLK_ENB_GPIO = 8, // Enable clock to GPIO controller - enabled on reset
    CLK_ENB_SDMMC2 = 9, // Enable clock to SDMMC2 controller
    CLK_ENB_SPDIF = 10, // Enable clock to S/PDIF controller
    CLK_ENB_I2S2 = 11, // Enable clock to I2S2 controller
    CLK_ENB_I2C1 = 12, // Enable clock to I2C1 controller
    /* 13 */
    CLK_ENB_SDMMC1 = 14, // Enable clock to SDMMC1 controller
    CLK_ENB_SDMMC4 = 15, // Enable clock to SDMMC4 controller
    /* 16 */
    CLK_ENB_PWM = 17, // Enable clock to PWM (Pulse Width Modulator)
    CLK_ENB_I2S3 = 18, // Enable clock to I2S3 controller
    /* 19 */
    CLK_ENB_VI = 20, // Enable clock to VI controller
    /* 21 */
    CLK_ENB_USBD = 22, // Enable clock to USB controller
    CLK_ENB_ISP = 23, // Enable clock to ISP controller
    /* 24 */
    /* 25 */
    CLK_ENB_DISP2 = 26, // Enable clock to DISP2 controller
    CLK_ENB_DISP1 = 27, // Enable clock to DISP1 controller
    CLK_ENB_HOST1X = 28, // Enable clock to Host1x
    /* 29 */
    CLK_ENB_I2S = 30, // Enable clock to I2S` controller
    CLK_ENB_CACHE2 = 31, // Enable clock to BPMP-Lite cache controller - enabled on reset
    CLK_ENB_MEM = 32, // Enable clock to MC
    CLK_ENB_AHBDMA = 33, // Enable clock to AHB-DMA
    CLK_ENB_APBDMA = 34, // Enable clocl to APB-DMA
    /* 35 */
    /* 36 */
    CLK_ENB_STAT_MON = 37, // Enable clock to statistic monitor
    CLK_ENB_PMC = 38, // Enable clock to PMC controller
    CLK_ENB_FUSE = 39, // Enable clock to FUSE controller and host2jtag clock - enabled on reset
    CLK_ENB_KFUSE = 40, // Enable clock to KFUSE controller
    CLK_ENB_SPI1 = 41, // Enable clock to SPI1 controller
    /* 42 */
    CLK_ENB_JTAG2BTC = 43, // Enable clock to jtag2btc interface - enabled on reset
    CLK_ENB_SPI2 = 44, // Enable clock to SPI2 controller
    /* 45 */
    CLK_ENB_SPI3 = 46, // Enable clock to SPI3 controller
    CLK_ENB_I2C5 = 47, // Enable clock to I2C5 controller
    CLK_ENB_DSI = 48, // Enable clock to DSI controller
    /* 49 */
    /* 50 */
    /* 51 */
    CLK_ENB_CSI = 52, // Enable clock to CSI controller
    /* 53 */
    CLK_ENB_I2C2 = 54, // Enable clock to I2C2 controller
    CLK_ENB_UARTC = 55, // Enable clock to UARTC controller
    CLK_ENB_MIPI_CAL = 56, // Enable clock to MIPI CAL logic
    CLK_ENB_EMC = 57, // Enable clock to MC/EMC controller
    CLK_ENB_USB2 = 58, // Enable clock to USB2 controller
    /* 59 */
    /* 60 */
    /* 61 */
    /* 62 */
    CLK_ENB_BSEV = 63, // Enable clock to BSEV controller
    /* 64 */
    CLK_ENB_UARTD = 65, // Enable clock to UARTD
    /* 66 */
    CLK_ENB_I2C3 = 67, // Enable clock to I2C3
    CLK_ENB_SPI4 = 68, // Enable clock to SPI4
    CLK_ENB_SDMMC3 = 69, //Enable clock to SDMMC3
    CLK_ENB_PCIE = 70, // Enable clock to PCIe
    /* 71 */
    CLK_ENB_AFI = 72, // Enable clock to AFI
    CLK_ENB_CSITE = 73, // Enable clock to CoreSight - enabled on reset
    /* 74 */
    /* 75 */
    CLK_ENB_LA = 76, // Enable clock to LA
    /* 77 */
    CLK_ENB_SOC_THERM = 78, // Enable clock to SOC_THERM controller
    CLK_ENB_DTV = 79, // Enable clock to DTV controller
    /* 80 */
    CLK_ENB_I2C_SLOW = 81, // Enable clock to I2C_SLOW
    CLK_ENB_DSIB = 82, // Enable clock to DSIB
    CLK_ENB_TSEC = 83, // Enable TSEC clock
    CLK_ENB_IRAMA = 84, // Enable IRAMA clock - enabled on reset
    CLK_ENB_IRAMB = 85, // Enable IRAMB clock - enabled on reset
    CLK_ENB_IRAMC = 86, // Enable IRAMB clock - enabled on reset
    CLK_ENB_IRAMD = 87, // Enable IRAMB clock - enabled on reset
    CLK_ENB_CRAM2 = 88, // Enable BPMP-Lite cache RAM clock - enabled on reset
    CLK_ENB_XUSB_HOST = 89, // Enable clock to XUSB HOST
    CLK_M_DOUBLER_ENB = 90, // Enable CLK_M clock doubler - enabled on reset
    /* 91 */
    CLK_ENB_SUS_OUT = 92, // Enable clock to SUS pad
    CLK_ENB_DEV2_OUT = 93, // Enable clock to DEV2 pad
    CLK_ENB_DEV1_OUT = 94, // Enable clock to DEV1 pad
    CLK_ENB_XUSB_DEV = 95, // Enable clock to XUSB DEV
    CLK_ENB_CPUG = 96, // Enable clock to CPUG
    /* 97 */
    /* 98 */
    CLK_ENB_MSELECT = 99, // Enable clock to MSELECT - enabled on reset
    CLK_ENB_TSENSOR = 100, // Enable clock to TSENSOR
    CLK_ENB_I2S4 = 101, // Enable clock to I2S4
    CLK_ENB_I2S5 = 102, // Enable clock to I2S5
    CLK_ENB_I2C4 = 103, // Enable clock to I2C4
    /* 104 */
    /* 105 */
    CLK_ENB_AHUB = 106, // Enable clock to AHUB
    CLK_ENB_APB2APE = 107, // Enable APB clock to APE
    /* 108 */
    /* 109 */
    /* 110 */
    CLK_ENB_HDA2CODEC_2X = 111, // Enable clock to HDA2CODEC_2X
    CLK_ENB_ATOMICS = 112, // Enable clock to ATOMICS
    /* 113 */
    /* 114 */
    /* 115 */
    /* 116 */
    /* 117 */
    CLK_ENB_SPDIF_DOUBLER = 118, // Enable S/PDIF audio sync clock doubler - enabled on reset
    CLK_ENB_ACTMON = 119, // Enable clock to ACTMON
    CLK_ENB_EXTPERIPH1 = 120, // Enable clock to EXTPERIPH1
    CLK_ENB_EXTPERIPH2 = 121, // Enable clock to EXTPERIPH2
    CLK_ENB_EXTPERIPH3 = 122, // Enable clock to EXTPERIPH3
    CLK_ENB_SATA_OOB = 123, // Enable clock to SATA_OOB
    CLK_ENB_SATA = 124, // Enable clock to SATA
    CLK_ENB_HDA = 125, // Enable clock to High Definition Audio
    /* 126 */
    /* 127 */
    CLK_ENB_HDA2HDMICODEC = 128, // Enable clock to HDA2HDMICODEC (no dedicated module or clock)
    CLK_ENB_RESERVED0 = 129, // Unused clock enable for satacoldrstn
    CLK_ENB_PCIERX0 = 130, // Enable clock to PCIERX0. CLK_ENB_PCIE is also the master control for all PCIERX* clocks.
    CLK_ENB_PCIERX1 = 131, // Enable clock to PCIERX1. CLK_ENB_PCIE is also the master control for all PCIERX* clocks.
    CLK_ENB_PCIERX2 = 132, // Enable clock to PCIERX2. CLK_ENB_PCIE is also the master control for all PCIERX* clocks.
    CLK_ENB_PCIERX3 = 133, // Enable clock to PCIERX3. CLK_ENB_PCIE is also the master control for all PCIERX* clocks.
    CLK_ENB_PCIERX4 = 134, // Enable clock to PCIERX4. CLK_ENB_PCIE is also the master control for all PCIERX* clocks.
    CLK_ENB_PCIERX5 = 135, // Enable clock to PCIERX5. CLK_ENB_PCIE is also the master control for all PCIERX* clocks.
    CLK_ENB_CEC = 136, // Enable clock to CEC
    CLK_ENB_PCIE2_IOBIST = 137, // Enable clock to PCIE2 IOBST
    CLK_ENB_EMC_IOBIST = 138, // Enable clock to EMC IOBIST
    /* 139 */
    CLK_ENB_SATA_IOBIST = 140, // Enable clock to SATA IOBIST
    CLK_ENB_MIPI_IOBIST = 141, // Enable clock to MIPI IOBIST
    /* 142 */
    CLK_ENB_XUSB = 143, // Enable clock to XUSB. IOBIST control has clock enable only.
    CLK_ENB_CILAB = 144, // Enable clock to CSI CILA and CILB
    CLK_ENB_CILCD = 145, // Enable clock to CSI CILC and CILD
    CLK_ENB_CILEF = 146, // Enable clock to CSI CILEF
    CLK_ENB_DSIA_LP = 147, // Enable clock to DSIA LP
    CLK_ENB_DSIB_LP = 148, // Enable clock to DSIB LP
    CLK_ENB_ENTROPY = 149, // Enable clock to ENTROPY - enabled on reset
    /* 150 */
    /* 151 */
    /* 152 */
    /* 153 */
    /* 154 */
    CLK_ENB_DVFS = 155, // Enable clock to CLDVFS
    CLK_ENB_XUSB_SS = 156, // Enable clock to XUSB_SS
    CLK_ENB_EMC_LATENCY = 157, // Enable clock to EMC latency
    CLK_ENB_MC1 = 158, // Enable clock to MC1
    /* 159 */
    CLK_ENB_SPARE = 160, // Enable clock - SPARE
    CLK_ENB_DMIC1 = 161, // Enable clock - DMIC1
    CLK_ENB_DMIC2 = 162, // Enable clock - DMIC2
    CLK_ENB_ETR = 163, // Enable ETR clock
    CLK_ENB_CAM_MCLK = 164, // Enable clock - CAM_MCLK
    CLK_ENB_CAM_MCLK2 = 165, // Enable clock - CAM_MCLK2
    CLK_ENB_I2C6 = 166, // Enable clock - I2C6
    CLK_ENB_MC_CAPA = 167, // Enable clock - MC daisy chain 1
    CLK_ENB_MC_CBPA = 168, // Enable clock - MC daisy chain 2
    CLK_ENB_MC_CPU = 169, // Enable clock - MC CPU
    CLK_ENB_MC_BBC = 170, // Enable clock - MC BBC
    CLK_ENB_VIM2_CLK = 171, // Enable clock - CAMERA ifc 2
    /* 172 */
    CLK_ENB_MIPIBIF = 173, // Reserved
    CLK_ENB_EMC_DLL = 174, // Enable clock - EMC DLL
    /* 175 */
    /* 176 */
    CLK_ENB_UART_FST_MIPI_CAL = 177, // Enable clock - UART_FST_MIPI_CAL
    CLK_ENB_VIC = 178, // Enable clock - VIC
    /* 179 */
    /* 180 */
    CLK_ENB_DPAUX = 181, // Enable clock - DPAUX
    CLK_ENB_SOR0 = 182, // Enable clock - SOR0
    CLK_ENB_SOR1 = 183, // Enable clock - SOR1
    CLK_ENB_GPU = 184, // Enable clock - GPU
    CLK_ENB_DBGAPB = 185, // Enable clock - DBGAPB
    CLK_ENB_HPLL_ADSP = 186, // Clock gate for HPLL (PLLC/PLLC2/PLLC3) branches going to ADSP
    CLK_ENB_PLLP_ADSP = 187, // Clock gate for PLLP branch going to ADSP
    CLK_ENB_PLLA_ADSP = 188, // Clock gate for PLLA branch going to ADSP
    CLK_ENB_PLLG_REF = 189, // Clock gate for reference clock branch going to PLLG
    /* 190 */
    /* 191 */
    CLK_ENB_SPARE1 = 193, // Enable clock - SPARE
    CLK_ENB_SDMMC_LEGACY = 194, // Enable clock - sdmcc_legacy_tm
    CLK_ENB_NVDEC = 195, // Enable NVDEC clock
    CLK_ENB_NVJPG = 196, // Enable NVJPG clock
    CLK_ENB_AXIAP = 197, // Enable AXIAP clock
    CLK_ENB_DMIC3 = 198, // Enable DMIC3 clock
    CLK_ENB_APE = 199, // Enable APE clock
    CLK_ENB_ADSP = 200, // Enable ADSP clock
    CLK_ENB_MC_CDPA = 201, // Enable clock - MC daisy chain4 - enabled on reset
    CLK_ENB_MC_CCPA = 202, // Enable clock - MC daisy chain3 - enabled on reset
    CLK_ENB_MAUD = 203, // Enable maud clock
    /* 204 */
    /* 205 */
    /* 206 */
    CLK_ENB_TSECB = 207, // Enable TSECB clock
    CLK_ENB_DPAUX1 = 208, // Enable DPAUX1 clock
    CLK_ENB_VI_I2C = 209, // Enable HSIC tracking clock
    CLK_ENB_HSIC_TRK = 210, // Enable HSIC tracking clock
    CLK_ENB_USB2_TRK = 211, // Enable USB2 tracking clock
    CLK_ENB_QSPI = 212, // Enable Quad SPI clock
    CLK_ENB_UARTAPE = 213, // Enable UART APE clock
    /* 214 */
    /* 215 */
    /* 216 */
    /* 217 */
    /* 218 */
    CLK_ENB_ADSPNEON = 219, // Enable ADSP neon clock
    CLK_ENB_NVENC = 220, // Enable NVENC clock
    CLK_ENB_IQC2 = 221, // Enable IQC2 clock
    CLK_ENB_IQC1 = 222, // Enable IQC1 clock
    CLK_ENB_SOR_SAFE = 223, // Enable SOR clock
    CLK_ENB_PLLP_OUT_CPU = 224, // Enable PLLP branches to CPU
    NCLOCKS
};

// clock gates are the same as the clock IDs
enum clock_gate {
    /* 0 */
    /* 1 */
    /* 2 */
    CLK_GATE_ENB_ISPB = 3, // Enable clock - ISPB
    CLK_GATE_ENB_RTC = 4, // Enable clock to RTC controller - enabled on reset
    CLK_GATE_ENB_TMR = 5, // Enable clock to Timer controller (sticky) - enabled on reset
    CLK_GATE_ENB_UARTA = 6, // Enable clock to UARTA controller
    CLK_GATE_ENB_UARTB = 7, // Enable clock to UARTB/VFIR controller
    CLK_GATE_ENB_GPIO = 8, // Enable clock to GPIO controller - enabled on reset
    CLK_GATE_ENB_SDMMC2 = 9, // Enable clock to SDMMC2 controller
    CLK_GATE_ENB_SPDIF = 10, // Enable clock to S/PDIF controller
    CLK_GATE_ENB_I2S2 = 11, // Enable clock to I2S2 controller
    CLK_GATE_ENB_I2C1 = 12, // Enable clock to I2C1 controller
    /* 13 */
    CLK_GATE_ENB_SDMMC1 = 14, // Enable clock to SDMMC1 controller
    CLK_GATE_ENB_SDMMC4 = 15, // Enable clock to SDMMC4 controller
    /* 16 */
    CLK_GATE_ENB_PWM = 17, // Enable clock to PWM (Pulse Width Modulator)
    CLK_GATE_ENB_I2S3 = 18, // Enable clock to I2S3 controller
    /* 19 */
    CLK_GATE_ENB_VI = 20, // Enable clock to VI controller
    /* 21 */
    CLK_GATE_ENB_USBD = 22, // Enable clock to USB controller
    CLK_GATE_ENB_ISP = 23, // Enable clock to ISP controller
    /* 24 */
    /* 25 */
    CLK_GATE_ENB_DISP2 = 26, // Enable clock to DISP2 controller
    CLK_GATE_ENB_DISP1 = 27, // Enable clock to DISP1 controller
    CLK_GATE_ENB_HOST1X = 28, // Enable clock to Host1x
    /* 29 */
    CLK_GATE_ENB_I2S = 30, // Enable clock to I2S` controller
    CLK_GATE_ENB_CACHE2 = 31, // Enable clock to BPMP-Lite cache controller - enabled on reset
    CLK_GATE_ENB_MEM = 32, // Enable clock to MC
    CLK_GATE_ENB_AHBDMA = 33, // Enable clock to AHB-DMA
    CLK_GATE_ENB_APBDMA = 34, // Enable clocl to APB-DMA
    /* 35 */
    /* 36 */
    CLK_GATE_ENB_STAT_MON = 37, // Enable clock to statistic monitor
    CLK_GATE_ENB_PMC = 38, // Enable clock to PMC controller
    CLK_GATE_ENB_FUSE = 39, // Enable clock to FUSE controller and host2jtag clock - enabled on reset
    CLK_GATE_ENB_KFUSE = 40, // Enable clock to KFUSE controller
    CLK_GATE_ENB_SPI1 = 41, // Enable clock to SPI1 controller
    /* 42 */
    CLK_GATE_ENB_JTAG2BTC = 43, // Enable clock to jtag2btc interface - enabled on reset
    CLK_GATE_ENB_SPI2 = 44, // Enable clock to SPI2 controller
    /* 45 */
    CLK_GATE_ENB_SPI3 = 46, // Enable clock to SPI3 controller
    CLK_GATE_ENB_I2C5 = 47, // Enable clock to I2C5 controller
    CLK_GATE_ENB_DSI = 48, // Enable clock to DSI controller
    /* 49 */
    /* 50 */
    /* 51 */
    CLK_GATE_ENB_CSI = 52, // Enable clock to CSI controller
    /* 53 */
    CLK_GATE_ENB_I2C2 = 54, // Enable clock to I2C2 controller
    CLK_GATE_ENB_UARTC = 55, // Enable clock to UARTC controller
    CLK_GATE_ENB_MIPI_CAL = 56, // Enable clock to MIPI CAL logic
    CLK_GATE_ENB_EMC = 57, // Enable clock to MC/EMC controller
    CLK_GATE_ENB_USB2 = 58, // Enable clock to USB2 controller
    /* 59 */
    /* 60 */
    /* 61 */
    /* 62 */
    CLK_GATE_ENB_BSEV = 63, // Enable clock to BSEV controller
    /* 64 */
    CLK_GATE_ENB_UARTD = 65, // Enable clock to UARTD
    /* 66 */
    CLK_GATE_ENB_I2C3 = 67, // Enable clock to I2C3
    CLK_GATE_ENB_SPI4 = 68, // Enable clock to SPI4
    CLK_GATE_ENB_SDMMC3 = 69, //Enable clock to SDMMC3
    CLK_GATE_ENB_PCIE = 70, // Enable clock to PCIe
    /* 71 */
    CLK_GATE_ENB_AFI = 72, // Enable clock to AFI
    CLK_GATE_ENB_CSITE = 73, // Enable clock to CoreSight - enabled on reset
    /* 74 */
    /* 75 */
    CLK_GATE_ENB_LA = 76, // Enable clock to LA
    /* 77 */
    CLK_GATE_ENB_SOC_THERM = 78, // Enable clock to SOC_THERM controller
    CLK_GATE_ENB_DTV = 79, // Enable clock to DTV controller
    /* 80 */
    CLK_GATE_ENB_I2C_SLOW = 81, // Enable clock to I2C_SLOW
    CLK_GATE_ENB_DSIB = 82, // Enable clock to DSIB
    CLK_GATE_ENB_TSEC = 83, // Enable TSEC clock
    CLK_GATE_ENB_IRAMA = 84, // Enable IRAMA clock - enabled on reset
    CLK_GATE_ENB_IRAMB = 85, // Enable IRAMB clock - enabled on reset
    CLK_GATE_ENB_IRAMC = 86, // Enable IRAMB clock - enabled on reset
    CLK_GATE_ENB_IRAMD = 87, // Enable IRAMB clock - enabled on reset
    CLK_GATE_ENB_CRAM2 = 88, // Enable BPMP-Lite cache RAM clock - enabled on reset
    CLK_GATE_ENB_XUSB_HOST = 89, // Enable clock to XUSB HOST
    CLK_GATE_M_DOUBLER_ENB = 90, // Enable CLK_M clock doubler - enabled on reset
    /* 91 */
    CLK_GATE_ENB_SUS_OUT = 92, // Enable clock to SUS pad
    CLK_GATE_ENB_DEV2_OUT = 93, // Enable clock to DEV2 pad
    CLK_GATE_ENB_DEV1_OUT = 94, // Enable clock to DEV1 pad
    CLK_GATE_ENB_XUSB_DEV = 95, // Enable clock to XUSB DEV
    CLK_GATE_ENB_CPUG = 96, // Enable clock to CPUG
    /* 97 */
    /* 98 */
    CLK_GATE_ENB_MSELECT = 99, // Enable clock to MSELECT - enabled on reset
    CLK_GATE_ENB_TSENSOR = 100, // Enable clock to TSENSOR
    CLK_GATE_ENB_I2S4 = 101, // Enable clock to I2S4
    CLK_GATE_ENB_I2S5 = 102, // Enable clock to I2S5
    CLK_GATE_ENB_I2C4 = 103, // Enable clock to I2C4
    /* 104 */
    /* 105 */
    CLK_GATE_ENB_AHUB = 106, // Enable clock to AHUB
    CLK_GATE_ENB_APB2APE = 107, // Enable APB clock to APE
    /* 108 */
    /* 109 */
    /* 110 */
    CLK_GATE_ENB_HDA2CODEC_2X = 111, // Enable clock to HDA2CODEC_2X
    CLK_GATE_ENB_ATOMICS = 112, // Enable clock to ATOMICS
    /* 113 */
    /* 114 */
    /* 115 */
    /* 116 */
    /* 117 */
    CLK_GATE_ENB_SPDIF_DOUBLER = 118, // Enable S/PDIF audio sync clock doubler - enabled on reset
    CLK_GATE_ENB_ACTMON = 119, // Enable clock to ACTMON
    CLK_GATE_ENB_EXTPERIPH1 = 120, // Enable clock to EXTPERIPH1
    CLK_GATE_ENB_EXTPERIPH2 = 121, // Enable clock to EXTPERIPH2
    CLK_GATE_ENB_EXTPERIPH3 = 122, // Enable clock to EXTPERIPH3
    CLK_GATE_ENB_SATA_OOB = 123, // Enable clock to SATA_OOB
    CLK_GATE_ENB_SATA = 124, // Enable clock to SATA
    CLK_GATE_ENB_HDA = 125, // Enable clock to High Definition Audio
    /* 126 */
    /* 127 */
    CLK_GATE_ENB_HDA2HDMICODEC = 128, // Enable clock to HDA2HDMICODEC (no dedicated module or clock)
    CLK_GATE_ENB_RESERVED0 = 129, // Unused clock enable for satacoldrstn
    CLK_GATE_ENB_PCIERX0 = 130, // Enable clock to PCIERX0. CLK_ENB_PCIE is also the master control for all PCIERX* clocks.
    CLK_GATE_ENB_PCIERX1 = 131, // Enable clock to PCIERX1. CLK_ENB_PCIE is also the master control for all PCIERX* clocks.
    CLK_GATE_ENB_PCIERX2 = 132, // Enable clock to PCIERX2. CLK_ENB_PCIE is also the master control for all PCIERX* clocks.
    CLK_GATE_ENB_PCIERX3 = 133, // Enable clock to PCIERX3. CLK_ENB_PCIE is also the master control for all PCIERX* clocks.
    CLK_GATE_ENB_PCIERX4 = 134, // Enable clock to PCIERX4. CLK_ENB_PCIE is also the master control for all PCIERX* clocks.
    CLK_GATE_ENB_PCIERX5 = 135, // Enable clock to PCIERX5. CLK_ENB_PCIE is also the master control for all PCIERX* clocks.
    CLK_GATE_ENB_CEC = 136, // Enable clock to CEC
    CLK_GATE_ENB_PCIE2_IOBIST = 137, // Enable clock to PCIE2 IOBST
    CLK_GATE_ENB_EMC_IOBIST = 138, // Enable clock to EMC IOBIST
    /* 139 */
    CLK_GATE_ENB_SATA_IOBIST = 140, // Enable clock to SATA IOBIST
    CLK_GATE_ENB_MIPI_IOBIST = 141, // Enable clock to MIPI IOBIST
    /* 142 */
    CLK_GATE_ENB_XUSB = 143, // Enable clock to XUSB. IOBIST control has clock enable only.
    CLK_GATE_ENB_CILAB = 144, // Enable clock to CSI CILA and CILB
    CLK_GATE_ENB_CILCD = 145, // Enable clock to CSI CILC and CILD
    CLK_GATE_ENB_CILEF = 146, // Enable clock to CSI CILEF
    CLK_GATE_ENB_DSIA_LP = 147, // Enable clock to DSIA LP
    CLK_GATE_ENB_DSIB_LP = 148, // Enable clock to DSIB LP
    CLK_GATE_ENB_ENTROPY = 149, // Enable clock to ENTROPY - enabled on reset
    /* 150 */
    /* 151 */
    /* 152 */
    /* 153 */
    /* 154 */
    CLK_GATE_ENB_DVFS = 155, // Enable clock to CLDVFS
    CLK_GATE_ENB_XUSB_SS = 156, // Enable clock to XUSB_SS
    CLK_GATE_ENB_EMC_LATENCY = 157, // Enable clock to EMC latency
    CLK_GATE_ENB_MC1 = 158, // Enable clock to MC1
    /* 159 */
    CLK_GATE_ENB_SPARE = 160, // Enable clock - SPARE
    CLK_GATE_ENB_DMIC1 = 161, // Enable clock - DMIC1
    CLK_GATE_ENB_DMIC2 = 162, // Enable clock - DMIC2
    CLK_GATE_ENB_ETR = 163, // Enable ETR clock
    CLK_GATE_ENB_CAM_MCLK = 164, // Enable clock - CAM_MCLK
    CLK_GATE_ENB_CAM_MCLK2 = 165, // Enable clock - CAM_MCLK2
    CLK_GATE_ENB_I2C6 = 166, // Enable clock - I2C6
    CLK_GATE_ENB_MC_CAPA = 167, // Enable clock - MC daisy chain 1
    CLK_GATE_ENB_MC_CBPA = 168, // Enable clock - MC daisy chain 2
    CLK_GATE_ENB_MC_CPU = 169, // Enable clock - MC CPU
    CLK_GATE_ENB_MC_BBC = 170, // Enable clock - MC BBC
    CLK_GATE_ENB_VIM2_CLK = 171, // Enable clock - CAMERA ifc 2
    /* 172 */
    CLK_GATE_ENB_MIPIBIF = 173, // Reserved
    CLK_GATE_ENB_EMC_DLL = 174, // Enable clock - EMC DLL
    /* 175 */
    /* 176 */
    CLK_GATE_ENB_UART_FST_MIPI_CAL = 177, // Enable clock - UART_FST_MIPI_CAL
    CLK_GATE_ENB_VIC = 178, // Enable clock - VIC
    /* 179 */
    /* 180 */
    CLK_GATE_ENB_DPAUX = 181, // Enable clock - DPAUX
    CLK_GATE_ENB_SOR0 = 182, // Enable clock - SOR0
    CLK_GATE_ENB_SOR1 = 183, // Enable clock - SOR1
    CLK_GATE_ENB_GPU = 184, // Enable clock - GPU
    CLK_GATE_ENB_DBGAPB = 185, // Enable clock - DBGAPB
    CLK_GATE_ENB_HPLL_ADSP = 186, // Clock gate for HPLL (PLLC/PLLC2/PLLC3) branches going to ADSP
    CLK_GATE_ENB_PLLP_ADSP = 187, // Clock gate for PLLP branch going to ADSP
    CLK_GATE_ENB_PLLA_ADSP = 188, // Clock gate for PLLA branch going to ADSP
    CLK_GATE_ENB_PLLG_REF = 189, // Clock gate for reference clock branch going to PLLG
    /* 190 */
    /* 191 */
    CLK_GATE_ENB_SPARE1 = 193, // Enable clock - SPARE
    CLK_GATE_ENB_SDMMC_LEGACY = 194, // Enable clock - sdmcc_legacy_tm
    CLK_GATE_ENB_NVDEC = 195, // Enable NVDEC clock
    CLK_GATE_ENB_NVJPG = 196, // Enable NVJPG clock
    CLK_GATE_ENB_AXIAP = 197, // Enable AXIAP clock
    CLK_GATE_ENB_DMIC3 = 198, // Enable DMIC3 clock
    CLK_GATE_ENB_APE = 199, // Enable APE clock
    CLK_GATE_ENB_ADSP = 200, // Enable ADSP clock
    CLK_GATE_ENB_MC_CDPA = 201, // Enable clock - MC daisy chain4 - enabled on reset
    CLK_GATE_ENB_MC_CCPA = 202, // Enable clock - MC daisy chain3 - enabled on reset
    CLK_GATE_ENB_MAUD = 203, // Enable maud clock
    /* 204 */
    /* 205 */
    /* 206 */
    CLK_GATE_ENB_TSECB = 207, // Enable TSECB clock
    CLK_GATE_ENB_DPAUX1 = 208, // Enable DPAUX1 clock
    CLK_GATE_ENB_VI_I2C = 209, // Enable HSIC tracking clock
    CLK_GATE_ENB_HSIC_TRK = 210, // Enable HSIC tracking clock
    CLK_GATE_ENB_USB2_TRK = 211, // Enable USB2 tracking clock
    CLK_GATE_ENB_QSPI = 212, // Enable Quad SPI clock
    CLK_GATE_ENB_UARTAPE = 213, // Enable UART APE clock
    /* 214 */
    /* 215 */
    /* 216 */
    /* 217 */
    /* 218 */
    CLK_GATE_ENB_ADSPNEON = 219, // Enable ADSP neon clock
    CLK_GATE_ENB_NVENC = 220, // Enable NVENC clock
    CLK_GATE_ENB_IQC2 = 221, // Enable IQC2 clock
    CLK_GATE_ENB_IQC1 = 222, // Enable IQC1 clock
    CLK_GATE_ENB_SOR_SAFE = 223, // Enable SOR clock
    CLK_GATE_ENB_PLLP_OUT_CPU = 224, // Enable PLLP branches to CPU
    NCLKGATES
};

// For the clock controller, only the CLK_OUT_ENB_L/H/U/V/W/X/Y registers are relevant.
// They allow to enable/disable a respective module's clock.
// Corresponding offsets are listed below.
enum clk_ctrl_register {
    CLK_RST_CONTROLLER_CLK_OUT_ENB_L_0 = 0x10,
    CLK_RST_CONTROLLER_CLK_OUT_ENB_H_0 = 0x14,
    CLK_RST_CONTROLLER_CLK_OUT_ENB_U_0 = 0x18,
    CLK_RST_CONTROLLER_CLK_OUT_ENB_V_0 = 0x360,
    CLK_RST_CONTROLLER_CLK_OUT_ENB_W_0 = 0x364,
    CLK_RST_CONTROLLER_CLK_OUT_ENB_X_0 = 0x280,
    CLK_RST_CONTROLLER_CLK_OUT_ENB_Y_0 = 0x298,
};

/* Alternate method for programming the CLK_OUT_ENB_L/H/U/V/W/X/Y registers: use the
 * corresponding CLK_ENB_L/H/U/V/W/X/Y_SET/CLR registers
 * - CLK_RST_CONTROLLER_CLK_ENB_L_SET_0 - Offset: 0x320
 * - CLK_RST_CONTROLLER_CLK_ENB_L_CLR_0 - Offset: 0x324
 * - CLK_RST_CONTROLLER_CLK_ENB_H_SET_0 - Offset: 0x328
 * - CLK_RST_CONTROLLER_CLK_ENB_H_CLR_0 - Offset: 0x32c
 * - CLK_RST_CONTROLLER_CLK_ENB_U_SET_0 - Offset: 0x330
 * - CLK_RST_CONTROLLER_CLK_ENB_U_CLR_0 - Offset: 0x334
 * - CLK_RST_CONTROLLER_CLK_ENB_V_SET_0 - Offset: 0x440
 * - CLK_RST_CONTROLLER_CLK_ENB_V_CLR_0 - Offset: 0x444
 * - CLK_RST_CONTROLLER_CLK_ENB_W_SET_0 - Offset: 0x448
 * - CLK_RST_CONTROLLER_CLK_ENB_W_CLR_0 - Offset: 0x44c
 * - CLK_RST_CONTROLLER_CLK_ENB_X_SET_0 - Offset: 0x284
 * - CLK_RST_CONTROLLER_CLK_ENB_X_CLR_0 - Offset: 0x288
 * - CLK_RST_CONTROLLER_CLK_ENB_Y_SET_0 - Offset: 0x29c
 * - CLK_RST_CONTROLLER_CLK_ENB_Y_CLR_0 - Offset: 0x2a0
*/

// For setting a respective clock source, etc., separate CLK_SOURCE_<MODULE> registers are relevant.
// Corresponding offsets are listed below.
// Currently only filled for UART, further modules have to be added based on TRM chapter 5.2 if required.
enum clk_src_register {
    CLK_RST_CONTROLLER_CLK_SOURCE_UARTA_0 = 0x178,
    CLK_RST_CONTROLLER_CLK_SOURCE_UARTB_0 = 0x17c,
    CLK_RST_CONTROLLER_CLK_SOURCE_UARTC_0 = 0x1a0,
    CLK_RST_CONTROLLER_CLK_SOURCE_UARTD_0 = 0x1c0,
};