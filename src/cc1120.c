#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "cc1120.h"
#include "dbg.h"

#define DBG_TAG		"CC1120"

uint8_t a_CC112X_FREQ2[21] =
{
	0x6A,0x6A,0x6A,0x6A,0x6A,0x6A,0x6A,0x6A,0x6A,0x6A,
	0x6A,0x6A,0x6A,0x6A,0x6A,0x6A,0x6A,0x6A,0x6A,0x6A,0x6A
};
uint8_t a_CC112X_FREQ1[21] =
{
	0x2C,0x2D,0x2E,0x2F,0x30,0x30,0x31,0x32,0x33,0x34,
	0x34,0x35,0x36,0x37,0x38,0x38,0x39,0x3A,0x3B,0x3C,0x3C
};
uint8_t a_CC112X_FREQ0[21] =
{
	0xCD,0x99,0x66,0x33,0x00,0xCD,0x99,0x66,0x33,0x00,
	0xCD,0x99,0x66,0x33,0x00,0xCD,0x99,0x66,0x33,0x00,0xCD
};

uint8_t a_CC112X_PA_CFG2[16] =
{
	0x5D,0x5F,0x62,0x64,0x66,0x69,0x6B,0x6D,0x6F,0x72,
	0x74,0x77,0x79,0x7B,0x7D,0x7F
};
uint8_t a_CC112X_PA_CFG1[16] =
{
	0x56,0x56,0x56,0x56,0x56,0x56,0x56,0x56,0x56,0x56,
	0x56,0x56,0x56,0x56,0x56,0x56};
uint8_t a_CC112X_PA_CFG0[16] = 
{
	0x3C,0x3C,0x44,0x4C,0x4C,0x54,0x54,0x5C,0x5C,0x64,
	0x6C,0x6C,0x74,0x74,0x7C,0x7C
};

//========================================
// CC1120 API
//========================================
void cc1120_hw_reset (int fd)
{
	ioctl(fd, RT2880_SPI_CC1120_RESET, NULL);
}

uint8_t cc1120_strobe (int fd, uint8_t cmd)
{
	SPI_CC1120 cc1120;

	cc1120.addr = cmd;
	ioctl(fd, RT2880_SPI_CC1120_STROBE, &cc1120);

	return cc1120.buf[0];
}

int cc1120_read (int fd, uint16_t addr, uint8_t *buf, int len)
{
	SPI_CC1120 cc1120;
	int i;

	cc1120.addr = addr;
	cc1120.len = len;
	ioctl(fd, RT2880_SPI_CC1120_READ, &cc1120);

	for (i=0; i<len; i++)
		buf[i] = cc1120.buf[i];

	return 0;
}

int cc1120_write (int fd, uint16_t addr, uint8_t *buf,  int wrlen)
{
	SPI_CC1120 cc1120;
	int i;

	cc1120.addr = addr;
	cc1120.len = wrlen;
	for (i=0; i<wrlen; i++)
		cc1120.buf[i] = buf[i];

	ioctl(fd, RT2880_SPI_CC1120_WRITE, &cc1120);

	return 0;
}

uint8_t cc1120_read_single (int fd, uint16_t addr)
{
	uint8_t val;
	cc1120_read (fd, addr, &val, 1);
	return val;
}

void cc1120_write_single (int fd, uint16_t addr, uint8_t val)
{
	cc1120_write (fd, addr, &val, 1);
}

int cc1120_fifo_read (int fd, uint8_t *buf, int len)
{
	SPI_CC1120 cc1120;
	int i;

	cc1120.len = len;
	ioctl(fd, RT2880_SPI_CC1120_FIFO_READ, &cc1120);

	for (i=0; i<len; i++)
		buf[i] = cc1120.buf[i];

	return 0;
}

int cc1120_fifo_write (int fd, uint8_t *buf,  int len)
{
	SPI_CC1120 cc1120;
	int i;

	cc1120.len = len;
	for (i=0; i<len; i++)
		cc1120.buf[i] = buf[i];

	ioctl(fd, RT2880_SPI_CC1120_FIFO_WRITE, &cc1120);

	return 0;
}

int cc1120_fifo_dread (int fd, uint8_t addr, uint8_t *buf, int len)
{
	SPI_CC1120 cc1120;
	int i;

	cc1120.len = len;
	cc1120.addr = addr;
	ioctl(fd, RT2880_SPI_CC1120_FIFO_DREAD, &cc1120);

	for (i=0; i<len; i++)
		buf[i] = cc1120.buf[i];

	return 0;
}

int cc1120_fifo_dwrite (int fd, uint8_t addr, uint8_t *buf,  int len)
{
	SPI_CC1120 cc1120;
	int i;

	cc1120.len = len;
	cc1120.addr = addr;
	for (i=0; i<len; i++)
		cc1120.buf[i] = buf[i];

	ioctl(fd, RT2880_SPI_CC1120_FIFO_DWRITE, &cc1120);

	return 0;
}

#if 0
void cc1120_send_packet (int fd, int txlen, uint8_t *buf)
{
	uint8_t loop;

	// be sure radio is in IDLE state
	cc1120_strobe (fd, CC112X_SIDLE);
	while ((cc1120_strobe(fd, CC112X_SNOP) & 0xf0) != CC112X_STATE_IDLE);

	// write array to fifio
	cc1120_write (fd, CC112X_RXTXFIFO, buf, txlen);

	// wait for packet to be sent    
	while ((cc1120_strobe (fd, CC112X_SIDLE) & 0xF0) != CC112X_STATE_TX)
	{
		usleep(1000);
	}
}
#endif

int cc1120_set_ch (int fd, int ch)
{
	//Frequency Configuration [23:16]
	cc1120_write_single (fd, CC112X_FREQ2, a_CC112X_FREQ2[ch]);
	cc1120_write_single (fd, CC112X_FREQ1, a_CC112X_FREQ1[ch]);
	cc1120_write_single (fd, CC112X_FREQ0, a_CC112X_FREQ0[ch]);

	cc1120_manualcalibration (fd);
	cc1120_strobe (fd, CC112X_SFRX);
	cc1120_strobe (fd, CC112X_SFTX);

	return 0;
}

int cc1120_set_power (int fd, int power)
{
	//Power Amplifier Configuration Reg. 2
	cc1120_write_single (fd, CC112X_PA_CFG2,a_CC112X_PA_CFG2[power]);
	cc1120_write_single (fd, CC112X_PA_CFG1,a_CC112X_PA_CFG1[power]);
	cc1120_write_single (fd, CC112X_PA_CFG0,a_CC112X_PA_CFG0[power]);

	cc1120_manualcalibration (fd);
	cc1120_strobe (fd, CC112X_SFRX);
	cc1120_strobe (fd, CC112X_SFTX);

	return 0;
}

int cc1120_get_rssi (int fd, int *rssi)
{
	uint8_t val;
	int retry = 0;

	// TODO : check !!!!!!!!!!!!!!!!!!!!!!!!!
	cc1120_strobe (fd, CC112X_SRX);
	do 
	{
		usleep (1000);
		if (retry++ > 10)
			return -1;

		val = cc1120_read_single (fd, CC112X_RSSI0);
		_dbg (DBG_TAG, "val : 0x%02x\n", val);
	} while (!(val & 0x01));

	val = cc1120_read_single (fd, CC112X_RSSI1);
	// TODO : rssi offset
	//	*rssi = val - cc1120cc1190RssiOffset;
	*rssi = (signed char)val;

	_dbg (DBG_TAG, "RSSI1 : %d\n", *rssi);

	return 0;
}

// Calibrates the RC Oclillator used for the cc112x wake on radio functionality.
void cc1120_calibrate_rcosc(int fd)
{
	uint8_t val;

	val = cc1120_read_single (fd, CC112X_WOR_CFG0);

	// mask register bitfields and write new values
	val = (val & 0xf9) | (0x02 << 1);
	cc1120_write_single(fd, CC112X_WOR_CFG0, val);

	// strobe idle to calibrate RC osc
	cc1120_strobe (fd, CC112X_SIDLE);

	//disable RC calibration
	val = (val & 0xf9) | (0x00 << 1);
	cc1120_write_single(fd, CC112X_WOR_CFG0, val);
}

#define VCDAC_START_OFFSET	2
#define FS_VCO2_INDEX		0
#define FS_VCO4_INDEX		1
#define FS_CHP_INDEX		2
void cc1120_manualcalibration (int fd)
{
	uint8_t original_fs_cal2;
	uint8_t calResults_for_vcdac_start_high[3];
	uint8_t calResults_for_vcdac_start_mid[3];
	uint8_t marcstate;
	uint8_t writeByte;

	// set idle
	cc1120_strobe (fd, CC112X_SIDLE);

	// 1) Set VCO cap-array to 0 (FS_VCO2 = 0x00)
	writeByte = 0x00;
	cc1120_write_single (fd, CC112X_FS_VCO2, writeByte);

	// 2) Start with high VCDAC (original VCDAC_START + 2):
	original_fs_cal2 = cc1120_read_single (fd, CC112X_FS_CAL2);
	writeByte = original_fs_cal2 + VCDAC_START_OFFSET;
	cc1120_write_single (fd, CC112X_FS_CAL2, writeByte);

	// 3) Calibrate and wait for calibration to be done (radio back in IDLE state)
	cc1120_strobe (fd, CC112X_SCAL);

	int timeout = 0;
	do 
	{
		marcstate = cc1120_read_single (fd, CC112X_MARCSTATE);
//		_dbg (DBG_TAG, "marcstate : %x\n", marcstate);
	} while (marcstate != 0x41 && timeout++ < 50);

	// 4) Read FS_VCO2, FS_VCO4 and FS_CHP register obtained with high VCDAC_START value
	calResults_for_vcdac_start_high[FS_VCO2_INDEX] = cc1120_read_single (fd, CC112X_FS_VCO2);
	calResults_for_vcdac_start_high[FS_VCO4_INDEX] = cc1120_read_single (fd, CC112X_FS_VCO4);
	calResults_for_vcdac_start_high[FS_CHP_INDEX] = cc1120_read_single (fd, CC112X_FS_CHP);

	// 5) Set VCO cap-array to 0 (FS_VCO2 = 0x00)
	writeByte = 0x00;
	cc1120_write_single (fd, CC112X_FS_VCO2, writeByte);

	// 6) Continue with mid VCDAC (original VCDAC_START):
	writeByte = original_fs_cal2;
	cc1120_write_single (fd, CC112X_FS_CAL2, writeByte);

	// 7) Calibrate and wait for calibration to be done (radio back in IDLE state)
	cc1120_strobe (fd, CC112X_SCAL);

	do 
	{
		marcstate = cc1120_read_single (fd, CC112X_MARCSTATE);
	} while (marcstate != 0x41);

	// 8) Read FS_VCO2, FS_VCO4 and FS_CHP register obtained with mid VCDAC_START value
	calResults_for_vcdac_start_mid[FS_VCO2_INDEX] = cc1120_read_single (fd, CC112X_FS_VCO2);
	calResults_for_vcdac_start_mid[FS_VCO4_INDEX] = cc1120_read_single (fd, CC112X_FS_VCO4);
	calResults_for_vcdac_start_mid[FS_CHP_INDEX] = cc1120_read_single (fd, CC112X_FS_CHP);

	// 9) Write back highest FS_VCO2 and corresponding FS_VCO and FS_CHP result
	if (calResults_for_vcdac_start_high[FS_VCO2_INDEX] > calResults_for_vcdac_start_mid[FS_VCO2_INDEX]) 
	{
		writeByte = calResults_for_vcdac_start_high[FS_VCO2_INDEX];
		cc1120_write_single (fd, CC112X_FS_VCO2, writeByte);
		writeByte = calResults_for_vcdac_start_high[FS_VCO4_INDEX];
		cc1120_write_single (fd, CC112X_FS_VCO4, writeByte);
		writeByte = calResults_for_vcdac_start_high[FS_CHP_INDEX];
		cc1120_write_single (fd, CC112X_FS_CHP, writeByte);
	}
	else 
	{
		writeByte = calResults_for_vcdac_start_mid[FS_VCO2_INDEX];
		cc1120_write_single (fd, CC112X_FS_VCO2, writeByte);
		writeByte = calResults_for_vcdac_start_mid[FS_VCO4_INDEX];
		cc1120_write_single (fd, CC112X_FS_VCO4, writeByte);
		writeByte = calResults_for_vcdac_start_mid[FS_CHP_INDEX];
		cc1120_write_single (fd, CC112X_FS_CHP, writeByte);
	}
}

// Rf settings for CC1120
// 424.7MHz, 2-FSK, Deviation-1.5kHz, Tx 5dBm, 
// PA ramping, High Performance
void cc1120_writesetting (int fd)
{
	cc1120_write_single (fd, CC112X_IOCFG3,0x06);        //GPIO3 IO Pin Configuration
	cc1120_write_single (fd, CC112X_IOCFG2,0x06);        //GPIO2 IO Pin Configuration
	cc1120_write_single (fd, CC112X_IOCFG1,0xB0);        //GPIO1 IO Pin Configuration
	cc1120_write_single (fd, CC112X_IOCFG0,0x40);        //GPIO0 IO Pin Configuration
	cc1120_write_single (fd, CC112X_SYNC_CFG1,0x0B);     //Sync Word Detection Configuration Reg. 1
	// 2014.08.21 bygomma : 1.5k
#if 0
	cc1120_write_single (fd, CC112X_DEVIATION_M,0x89);    //Frequency Deviation Configuration
	cc1120_write_single (fd, CC112X_MODCFG_DEV_E,0x01);   //Modulation Format and Frequency Deviation Configur..
#endif
	// 2014.08.21 bygomma : 4k
	cc1120_write_single (fd, CC112X_DEVIATION_M,0x06);    //Frequency Deviation Configuration
	cc1120_write_single (fd, CC112X_MODCFG_DEV_E,0x03);   //Modulation Format and Frequency Deviation Configur..
	// 2014.08.21 bygomma : end

	cc1120_write_single (fd, CC112X_DCFILT_CFG,0x1C);    //Digital DC Removal Configuration
	cc1120_write_single (fd, CC112X_PREAMBLE_CFG1,0x18); //Preamble Length Configuration Reg. 1											 // 4Byte 0xAA
	cc1120_write_single (fd, CC112X_IQIC,0xC6);          //Digital Image Channel Compensation Configuration
	cc1120_write_single (fd, CC112X_CHAN_BW,0x10);       //Channel Filter Configuration
	cc1120_write_single (fd, CC112X_MDMCFG0,0x05);       //General Modem Parameter Configuration Reg. 0
	cc1120_write_single (fd, CC112X_AGC_REF,0x20);       //AGC Reference Level Configuration

#if 0
	// 2014.08.21 bygomma : 전파인증
	// cc1120_write_single (fd, CC112X_AGC_CS_THR,0x01);    //Carrier Sense Threshold Configuration
	cc1120_write_single (fd, CC112X_AGC_CS_THR,0x00);    //Carrier Sense Threshold Configuration
	_dbg (DBG_TAG, "cs_thr : %d\n", cc1120_read_single (fd, CC112X_AGC_CS_THR));
	// 2014.08.21 bygomma : end
#endif

	// 2014.12.04 bygomma : apply RSSI offset configuration
	cc1120_write_single (fd, CC112X_AGC_GAIN_ADJUST,0x9A);	// -102dB
	cc1120_write_single (fd, CC112X_AGC_CS_THR,0x9A);		// -102dB
	// 2014.12.04 bygomma : end

	// 2014.08.01 bygomma : LBT
	cc1120_write_single (fd, CC112X_PKT_CFG2,0x10);	// below RSSI threshold & ETSI LBT
	// 2014.08.01 bygomma : end

	cc1120_write_single (fd, CC112X_AGC_CFG1,0xA9);      //Automatic Gain Control Configuration Reg. 1
	cc1120_write_single (fd, CC112X_FIFO_CFG,0x00);      //FIFO Configuration
	cc1120_write_single (fd, CC112X_SETTLING_CFG,0x03);  //Frequency Synthesizer Calibration and Settling Con..
	cc1120_write_single (fd, CC112X_FS_CFG,0x14);        //Frequency Synthesizer Configuration
	cc1120_write_single (fd, CC112X_PKT_CFG0,0x20);      //Packet Configuration Reg. 0
	cc1120_write_single (fd, CC112X_PA_CFG2,0x69);        //Power Amplifier Configuration Reg. 2
	cc1120_write_single (fd, CC112X_PA_CFG0,0x7E);        //Power Amplifier Configuration Reg. 0
	cc1120_write_single (fd, CC112X_PKT_LEN,0xFF);       //Packet Length Configuration

	cc1120_write_single (fd, CC112X_IF_MIX_CFG,0x00);    //IF Mix Configuration
	cc1120_write_single (fd, CC112X_FREQOFF_CFG,0x22);   //Frequency Offset Correction Configuration
	cc1120_write_single (fd, CC112X_FREQ2,0x6A);         //Frequency Configuration [23:16]
	cc1120_write_single (fd, CC112X_FREQ1,0x2C);         //Frequency Configuration [15:8]
	cc1120_write_single (fd, CC112X_FREQ0,0xCD);         //Frequency Configuration [7:0]
	cc1120_write_single (fd, CC112X_FS_DIG1,0x00);       //Frequency Synthesizer Digital Reg. 1
	cc1120_write_single (fd, CC112X_FS_DIG0,0x5F);       //Frequency Synthesizer Digital Reg. 0
	cc1120_write_single (fd, CC112X_FS_CAL1,0x40);       //Frequency Synthesizer Calibration Reg. 1
	cc1120_write_single (fd, CC112X_FS_CAL0,0x0E);       //Frequency Synthesizer Calibration Reg. 0
	cc1120_write_single (fd, CC112X_FS_DIVTWO,0x03);     //Frequency Synthesizer Divide by 2
	cc1120_write_single (fd, CC112X_FS_DSM0,0x33);       //FS Digital Synthesizer Module Configuration Reg. 0
	cc1120_write_single (fd, CC112X_FS_DVC0,0x17);       //Frequency Synthesizer Divider Chain Configuration ..
	cc1120_write_single (fd, CC112X_FS_PFD,0x50);        //Frequency Synthesizer Phase Frequency Detector Con..
	cc1120_write_single (fd, CC112X_FS_PRE,0x6E);        //Frequency Synthesizer Prescaler Configuration
	cc1120_write_single (fd, CC112X_FS_REG_DIV_CML,0x14);//Frequency Synthesizer Divider Regulator Configurat..
	cc1120_write_single (fd, CC112X_FS_SPARE,0xAC);      //Frequency Synthesizer Spare
	cc1120_write_single (fd, CC112X_FS_VCO4,0x0E);        //FS Voltage Controlled Oscillator Configuration Reg..
	cc1120_write_single (fd, CC112X_FS_VCO1,0x9C);        //FS Voltage Controlled Oscillator Configuration Reg..
	cc1120_write_single (fd, CC112X_FS_VCO0,0xB4);        //FS Voltage Controlled Oscillator Configuration Reg..

	cc1120_write_single (fd, CC112X_XOSC5,0x0E);         //Crystal Oscillator Configuration Reg. 5
	cc1120_write_single (fd, CC112X_XOSC1,0x03);         //Crystal Oscillator Configuration Reg. 1

	// 2014.06.11 bygomma : ioc_sync_pins_en
	cc1120_write_single (fd, CC112X_SERIAL_STATUS, 0x08);    //Serail Status
	// 2014.06.11 bygomma : end
}
