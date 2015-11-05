#ifndef	_CC1120_H_
#define	_CC1120_H_

#include <stdint.h>

// ioctl 

typedef struct spi_cc1120_data {
	unsigned short addr;
	unsigned char len;
	unsigned char buf[256];
} SPI_CC1120;

#define RT2880_SPI_CC1120_RESET         20
#define RT2880_SPI_CC1120_STROBE        21
#define RT2880_SPI_CC1120_READ          22
#define RT2880_SPI_CC1120_WRITE         23
#define RT2880_SPI_CC1120_FIFO_READ     24      // FIFO standard read
#define RT2880_SPI_CC1120_FIFO_WRITE    25      // FIFO standard write
#define RT2880_SPI_CC1120_FIFO_DREAD    26      // FIFO direct read
#define RT2880_SPI_CC1120_FIFO_DWRITE   27      // FIFO direct write

#define READ_SINGLE		0x80
#define READ_BURST		0xC0
#define WRITE_SINGLE	0x00
#define WRITE_BURST		0x40

/* configuration registers */
#define CC112X_IOCFG3                   0x0000
#define CC112X_IOCFG2                   0x0001
#define CC112X_IOCFG1                   0x0002
#define CC112X_IOCFG0                   0x0003
#define CC112X_SYNC3                    0x0004
#define CC112X_SYNC2                    0x0005
#define CC112X_SYNC1                    0x0006
#define CC112X_SYNC0                    0x0007
#define CC112X_SYNC_CFG1                0x0008
#define CC112X_SYNC_CFG0                0x0009
#define CC112X_DEVIATION_M              0x000A
#define CC112X_MODCFG_DEV_E             0x000B
#define CC112X_DCFILT_CFG               0x000C
#define CC112X_PREAMBLE_CFG1            0x000D
#define CC112X_PREAMBLE_CFG0            0x000E
#define CC112X_FREQ_IF_CFG              0x000F
#define CC112X_IQIC                     0x0010
#define CC112X_CHAN_BW                  0x0011
#define CC112X_MDMCFG1                  0x0012
#define CC112X_MDMCFG0                  0x0013
#define CC112X_SYMBOL_RATE2             0x0014
#define CC112X_SYMBOL_RATE1             0x0015
#define CC112X_SYMBOL_RATE0             0x0016
#define CC112X_AGC_REF                  0x0017
#define CC112X_AGC_CS_THR               0x0018
#define CC112X_AGC_GAIN_ADJUST          0x0019
#define CC112X_AGC_CFG3                 0x001A
#define CC112X_AGC_CFG2                 0x001B
#define CC112X_AGC_CFG1                 0x001C
#define CC112X_AGC_CFG0                 0x001D
#define CC112X_FIFO_CFG                 0x001E
#define CC112X_DEV_ADDR                 0x001F
#define CC112X_SETTLING_CFG             0x0020
#define CC112X_FS_CFG                   0x0021
#define CC112X_WOR_CFG1                 0x0022
#define CC112X_WOR_CFG0                 0x0023
#define CC112X_WOR_EVENT0_MSB           0x0024
#define CC112X_WOR_EVENT0_LSB           0x0025
#define CC112X_PKT_CFG2                 0x0026
#define CC112X_PKT_CFG1                 0x0027
#define CC112X_PKT_CFG0                 0x0028
#define CC112X_RFEND_CFG1               0x0029
#define CC112X_RFEND_CFG0               0x002A
#define CC112X_PA_CFG2                  0x002B
#define CC112X_PA_CFG1                  0x002C
#define CC112X_PA_CFG0                  0x002D
#define CC112X_PKT_LEN                  0x002E

/* Extended Configuration Registers */
#define CC112X_IF_MIX_CFG               0x2F00
#define CC112X_FREQOFF_CFG              0x2F01
#define CC112X_TOC_CFG                  0x2F02
#define CC112X_MARC_SPARE               0x2F03
#define CC112X_ECG_CFG                  0x2F04
#define CC112X_CFM_DATA_CFG             0x2F05
#define CC112X_EXT_CTRL                 0x2F06
#define CC112X_RCCAL_FINE               0x2F07
#define CC112X_RCCAL_COARSE             0x2F08
#define CC112X_RCCAL_OFFSET             0x2F09
#define CC112X_FREQOFF1                 0x2F0A
#define CC112X_FREQOFF0                 0x2F0B
#define CC112X_FREQ2                    0x2F0C
#define CC112X_FREQ1                    0x2F0D
#define CC112X_FREQ0                    0x2F0E
#define CC112X_IF_ADC2                  0x2F0F
#define CC112X_IF_ADC1                  0x2F10
#define CC112X_IF_ADC0                  0x2F11
#define CC112X_FS_DIG1                  0x2F12
#define CC112X_FS_DIG0                  0x2F13
#define CC112X_FS_CAL3                  0x2F14
#define CC112X_FS_CAL2                  0x2F15
#define CC112X_FS_CAL1                  0x2F16
#define CC112X_FS_CAL0                  0x2F17
#define CC112X_FS_CHP                   0x2F18
#define CC112X_FS_DIVTWO                0x2F19
#define CC112X_FS_DSM1                  0x2F1A
#define CC112X_FS_DSM0                  0x2F1B
#define CC112X_FS_DVC1                  0x2F1C
#define CC112X_FS_DVC0                  0x2F1D
#define CC112X_FS_LBI                   0x2F1E
#define CC112X_FS_PFD                   0x2F1F
#define CC112X_FS_PRE                   0x2F20
#define CC112X_FS_REG_DIV_CML           0x2F21
#define CC112X_FS_SPARE                 0x2F22
#define CC112X_FS_VCO4                  0x2F23
#define CC112X_FS_VCO3                  0x2F24
#define CC112X_FS_VCO2                  0x2F25
#define CC112X_FS_VCO1                  0x2F26
#define CC112X_FS_VCO0                  0x2F27
#define CC112X_GBIAS6                   0x2F28
#define CC112X_GBIAS5                   0x2F29
#define CC112X_GBIAS4                   0x2F2A
#define CC112X_GBIAS3                   0x2F2B
#define CC112X_GBIAS2                   0x2F2C
#define CC112X_GBIAS1                   0x2F2D
#define CC112X_GBIAS0                   0x2F2E
#define CC112X_IFAMP                    0x2F2F
#define CC112X_LNA                      0x2F30
#define CC112X_RXMIX                    0x2F31
#define CC112X_XOSC5                    0x2F32
#define CC112X_XOSC4                    0x2F33
#define CC112X_XOSC3                    0x2F34
#define CC112X_XOSC2                    0x2F35
#define CC112X_XOSC1                    0x2F36
#define CC112X_XOSC0                    0x2F37
#define CC112X_ANALOG_SPARE             0x2F38
#define CC112X_PA_CFG3                  0x2F39
#define CC112X_IRQ0M                    0x2F3F
#define CC112X_IRQ0F                    0x2F40 

/* Status Registers */
#define CC112X_WOR_TIME1                0x2F64
#define CC112X_WOR_TIME0                0x2F65
#define CC112X_WOR_CAPTURE1             0x2F66
#define CC112X_WOR_CAPTURE0             0x2F67
#define CC112X_BIST                     0x2F68
#define CC112X_DCFILTOFFSET_I1          0x2F69
#define CC112X_DCFILTOFFSET_I0          0x2F6A
#define CC112X_DCFILTOFFSET_Q1          0x2F6B
#define CC112X_DCFILTOFFSET_Q0          0x2F6C
#define CC112X_IQIE_I1                  0x2F6D
#define CC112X_IQIE_I0                  0x2F6E
#define CC112X_IQIE_Q1                  0x2F6F
#define CC112X_IQIE_Q0                  0x2F70
#define CC112X_RSSI1                    0x2F71
#define CC112X_RSSI0                    0x2F72
#define CC112X_MARCSTATE                0x2F73
#define CC112X_LQI_VAL                  0x2F74
#define CC112X_PQT_SYNC_ERR             0x2F75
#define CC112X_DEM_STATUS               0x2F76
#define CC112X_FREQOFF_EST1             0x2F77
#define CC112X_FREQOFF_EST0             0x2F78
#define CC112X_AGC_GAIN3                0x2F79
#define CC112X_AGC_GAIN2                0x2F7A
#define CC112X_AGC_GAIN1                0x2F7B
#define CC112X_AGC_GAIN0                0x2F7C
#define CC112X_CFM_RX_DATA_OUT          0x2F7D
#define CC112X_CFM_TX_DATA_IN           0x2F7E
#define CC112X_ASK_SOFT_RX_DATA         0x2F7F
#define CC112X_RNDGEN                   0x2F80
#define CC112X_MAGN2                    0x2F81
#define CC112X_MAGN1                    0x2F82
#define CC112X_MAGN0                    0x2F83
#define CC112X_ANG1                     0x2F84
#define CC112X_ANG0                     0x2F85
#define CC112X_CHFILT_I2                0x2F86
#define CC112X_CHFILT_I1                0x2F87
#define CC112X_CHFILT_I0                0x2F88
#define CC112X_CHFILT_Q2                0x2F89
#define CC112X_CHFILT_Q1                0x2F8A
#define CC112X_CHFILT_Q0                0x2F8B
#define CC112X_GPIO_STATUS              0x2F8C
#define CC112X_FSCAL_CTRL               0x2F8D
#define CC112X_PHASE_ADJUST             0x2F8E
#define CC112X_PARTNUMBER               0x2F8F
#define CC112X_PARTVERSION              0x2F90
#define CC112X_SERIAL_STATUS            0x2F91
#define CC112X_MODEM_STATUS1            0x2F92
#define CC112X_MODEM_STATUS0            0x2F93
#define CC112X_MARC_STATUS1             0x2F94
#define CC112X_MARC_STATUS0             0x2F95
#define CC112X_PA_IFAMP_TEST            0x2F96
#define CC112X_FSRF_TEST                0x2F97
#define CC112X_PRE_TEST                 0x2F98
#define CC112X_PRE_OVR                  0x2F99
#define CC112X_ADC_TEST                 0x2F9A
#define CC112X_DVC_TEST                 0x2F9B
#define CC112X_ATEST                    0x2F9C
#define CC112X_ATEST_LVDS               0x2F9D
#define CC112X_ATEST_MODE               0x2F9E
#define CC112X_XOSC_TEST1               0x2F9F
#define CC112X_XOSC_TEST0               0x2FA0  
                                        
#define CC112X_RXFIRST                  0x2FD2   
#define CC112X_TXFIRST                  0x2FD3   
#define CC112X_RXLAST                   0x2FD4 
#define CC112X_TXLAST                   0x2FD5 
#define CC112X_NUM_TXBYTES              0x2FD6  /* Number of bytes in TXFIFO */ 
#define CC112X_NUM_RXBYTES              0x2FD7  /* Number of bytes in RXFIFO */
#define CC112X_FIFO_NUM_TXBYTES         0x2FD8  
#define CC112X_FIFO_NUM_RXBYTES         0x2FD9  
                                                                                                                                                
/* DATA FIFO Access */
#define CC112X_RXTXFIFO            		0x003F      /*  RXTXFIFO  */
#define CC112X_SINGLE_TXFIFO            0x003F      /*  TXFIFO  - Single accecss to Transmit FIFO */
#define CC112X_BURST_TXFIFO             0x007F      /*  TXFIFO  - Burst accecss to Transmit FIFO  */
#define CC112X_SINGLE_RXFIFO            0x00BF      /*  RXFIFO  - Single accecss to Receive FIFO  */
#define CC112X_BURST_RXFIFO             0x00FF      /*  RXFIFO  - Busrrst ccecss to Receive FIFO  */

#define CC112X_LQI_CRC_OK_BM            0x80
#define CC112X_LQI_EST_BM               0x7F

/* Command strobe registers */
#define CC112X_SRES                     0x30      /*  SRES    - Reset chip. */
#define CC112X_SFSTXON                  0x31      /*  SFSTXON - Enable and calibrate frequency synthesizer. */
#define CC112X_SXOFF                    0x32      /*  SXOFF   - Turn off crystal oscillator. */
#define CC112X_SCAL                     0x33      /*  SCAL    - Calibrate frequency synthesizer and turn it off. */
#define CC112X_SRX                      0x34      /*  SRX     - Enable RX. Perform calibration if enabled. */
#define CC112X_STX                      0x35      /*  STX     - Enable TX. If in RX state, only enable TX if CCA passes. */
#define CC112X_SIDLE                    0x36      /*  SIDLE   - Exit RX / TX, turn off frequency synthesizer. */
#define CC112X_SWOR                     0x38      /*  SWOR    - Start automatic RX polling sequence (Wake-on-Radio) */
#define CC112X_SPWD                     0x39      /*  SPWD    - Enter power down mode when CSn goes high. */
#define CC112X_SFRX                     0x3A      /*  SFRX    - Flush the RX FIFO buffer. */
#define CC112X_SFTX                     0x3B      /*  SFTX    - Flush the TX FIFO buffer. */
#define CC112X_SWORRST                  0x3C      /*  SWORRST - Reset real time clock. */
#define CC112X_SNOP                     0x3D      /*  SNOP    - No operation. Returns status byte. */
#define CC112X_AFC                      0x37      /*  AFC     - Automatic Frequency Correction */

/* Chip states returned in status byte */
#define CC112X_STATE_IDLE               0x00
#define CC112X_STATE_RX                 0x10
#define CC112X_STATE_TX                 0x20
#define CC112X_STATE_FSTXON             0x30
#define CC112X_STATE_CALIBRATE          0x40
#define CC112X_STATE_SETTLING           0x50
#define CC112X_STATE_RXFIFO_ERROR       0x60
#define CC112X_STATE_TXFIFO_ERROR       0x70

uint8_t cc1120_strobe (int fd, uint8_t cmd);
int cc1120_read (int fd, uint16_t addr, uint8_t *buf, int len);
int cc1120_write (int fd, uint16_t addr, uint8_t *buf, int len);
uint8_t cc1120_read_single (int fd, uint16_t addr);
void cc1120_write_single (int fd, uint16_t addr, uint8_t val);
int cc1120_fifo_write (int fd, uint8_t *buf,  int len);
int cc1120_fifo_read (int fd, uint8_t *buf, int len);
void cc1120_writesetting (int fd);
void cc1120_calibrate_rcosc(int fd);
void cc1120_manualcalibration (int fd);

/********************************************************************
FREQUNCY TABLE

* Data Transmission : 424.7000 ~ 424.9500快              
No Carrier Frequency(快) FREQ2 Reg. FREQ1 Reg. FREQ0 Reg.
0               424.7000      0x6A      0x2C      0xCD                                
1               424.7125      0x6A      0x2D      0x99                                
2               424.7250      0x6A      0x2E      0x66                                
3               424.7375      0x6A      0x2F      0x33                                
4               424.7500      0x6A      0x30      0x00                                
5               424.7625      0x6A      0x30      0xCD                                
6               424.7750      0x6A      0x31      0x99                                
7               424.7875      0x6A      0x32      0x66                                
8               424.8000      0x6A      0x33      0x33                                
9               424.8125      0x6A      0x34      0x00                                
10              424.8250      0x6A      0x34      0xCD                               
11              424.8375      0x6A      0x35      0x99                               
12              424.8500      0x6A      0x36      0x66                               
13              424.8625      0x6A      0x37      0x33                               
14              424.8750      0x6A      0x38      0x00                               
15              424.8875      0x6A      0x38      0xCD                               
16              424.9000      0x6A      0x39      0x99                               
17              424.9125      0x6A      0x3A      0x66                               
18              424.9250      0x6A      0x3B      0x33                               
19              424.9375      0x6A      0x3C      0x00                               
20              424.9500      0x6A      0x3C      0xCD


* Industrial : 447.8625 ~ 447.9875快                     
No Carrier Frequency(快) FREQ2 Reg. FREQ1 Reg. FREQ0 Reg.
1               447.8625      0x6F      0xF7      0x33                                
2               447.8750      0x6F      0xF8      0x00                                
3               447.8875      0x6F      0xF8      0xCD                                
4               447.9000      0x6F      0xF9      0x99                                
5               447.9125      0x6F      0xFA      0x66                                
6               447.9250      0x6F      0xFB      0x33                                
7               447.9375      0x6F      0xFC      0x00                                
8               447.9500      0x6F      0xFC      0xCD                                
8               447.9625      0x6F      0xFD      0x99                                
10              447.9750      0x6F      0xFE      0x66                               
11              447.9875      0x6F      0xFF      0x33


*Fire/Security : 447.2625MHz ? 447.5625快                
No Carrier Frequency(快) FREQ2 Reg. FREQ1 Reg. FREQ0 Reg.
0               447.2625      0x6F      0xD0      0xCD                                
1               447.2750      0x6F      0xD1      0x99                                
2               447.2875      0x6F      0xD2      0x66                                
3               447.3000      0x6F      0xD3      0x33                                
4               447.3125      0x6F      0xD4      0x00                                
5               447.3250      0x6F      0xD4      0xCD                                
6               447.3375      0x6F      0xD5      0x99                                
7               447.3500      0x6F      0xD6      0x66                                
8               447.3625      0x6F      0xD7      0x33                                
9               447.3750      0x6F      0xD8      0x00                                
10              447.3875      0x6F      0xD8      0xCD                               
11              447.4000      0x6F      0xD9      0x99                               
12              447.4125      0x6F      0xDA      0x66                               
13              447.4250      0x6F      0xDB      0x33                               
14              447.4375      0x6F      0xDC      0x00                               
15              447.4500      0x6F      0xDC      0xCD                               
16              447.4625      0x6F      0xDD      0x99                               
17              447.4750      0x6F      0xDE      0x66                               
18              447.4875      0x6F      0xDF      0x33                               
19              447.5000      0x6F      0xE0      0x00                               
20              447.5125      0x6F      0xE0      0xCD                               
21              447.5250      0x6F      0xE1      0x99                               
22              447.5375      0x6F      0xE2      0x66                               
23              447.5500      0x6F      0xE3      0x33                               
24              447.5625      0x6F      0xE4      0x00
                                        

5.8 CC1120 RF POWER TABLE                                
Power(dBm) PA_CFG2 Reg. PA_CFG1 Reg. PA_CFG0 Reg. 綠堅   
0 dBm             0x5D         0x56         0x3C ﹛                                  
1 dBm             0x5F         0x56         0x3C ﹛                                  
2 dBm             0x62         0x56         0x44 ﹛                                  
3 dBm             0x64         0x56         0x4C ﹛                                  
4 dBm             0x66         0x56         0x4C ﹛                                  
5 dBm             0x69         0x56         0x54 ﹛                                  
6 dBm             0x6B         0x56         0x54 ﹛                                  
7 dBm             0x6D         0x56         0x5C ﹛                                  
8 dBm             0x6F         0x56         0x5C ﹛                                  
9 dBm             0x72         0x56         0x64 ﹛                                  
10 dBm            0x74         0x56         0x6C ﹛                                 
11 dBm            0x77         0x56         0x6C ﹛                                 
12 dBm            0x79         0x56         0x74 ﹛                                 
13 dBm            0x7B         0x56         0x74 ﹛                                 
14 dBm            0x7D         0x56         0x7C ﹛                                 
15 dBm            0x7F         0x56         0x7C ﹛   

********************************************************************/

#endif 	// _CC1120_H_


