/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2009, Ralink Technology, Inc.
 *
 * All rights reserved. Ralink's source code is an unpublished work and the
 * use of a copyright notice does not imply otherwise. This source code
 * contains confidential trade secret material of Ralink Tech. Any attemp
 * or participation in deciphering, decoding, reverse engineering or in any
 * way altering the source code is stricitly prohibited, unless the prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

	Module Name:
	cmm_txbf_cal.c

	Abstract:
	Tx Beamforming calibration and profile related functions

	Revision History:
	Who         When          What
	--------    ----------    ----------------------------------------------
	Shiang     2010/07/12
*/

#include "rt_config.h"


/*
	iAtan2 - fixed point atan2. Returns +/-pi. Scaled so pi=0x1000
		Code was taken from MyCordic:
			int MyCordic(int y, int x, int shift, int iter, int *alpha)
			Parameters are hard coded so it's equivalent to MyCordic(y, x, 0, 11, alpha10);
*/
static int iAtan2(int y, int x)
{
	int z = 0, xtemp, i;
	int onepi, halfpi;
	int shift=0;
	int iter=11;
	static int alpha[11] = {0x400, 0x25c, 0x13f, 0x0a2,
				0x051, 0x028, 0x014, 0x00a,
				0x005, 0x002, 0x001};

	onepi = (alpha[0]<<2),
	halfpi = (alpha[0]<<1);

    if (x == 0) {
		if (y == 0)
              z = 0;
        else if (y > 0)
                z = halfpi;
        else
            z = -halfpi;
		}
    else if ((x < 0) && (y == 0)){
            z = -onepi;
		}
    else if ((x > 0) && (y == 0)){
            z = 0;
		}
	else{
		x <<= shift;
		y <<= shift;
		xtemp = x;
		if ((x < 0) && (y > 0)){
            x = y;
            y = -xtemp;
            z = halfpi;
			}
		else if ((x < 0) && (y < 0)){
            x = -y;
            y = xtemp;
            z = -halfpi;
			}
		for (i = 0; i < iter; i++){
			/* printf("%d %d %x\n", x, y, z); */
			if (y == 0)
				break;
			if (y < 0) {
				z -= alpha[i];
				xtemp = x - (y>>i);
				y = y + (x>>i);
				x = xtemp;
			}
			else {
				z += alpha[i];
				xtemp = x + (y>>i);
				y = y - (x>>i);
				x = xtemp;
				}
		}
	}

	if (z == alpha[0]*4)
		z = -(alpha[0]*4);

	return z;
}


/*
	isqrt - fixed point sqrt
		x - unsigned value
*/
static uint32_t isqrt (uint32_t x)
{
	uint32_t base, y;

	if (x &      0xF0000000)
		base = 1<<15;
	else if (x & 0x0F000000)
		base = 1<<13;
	else if (x & 0x00F00000)
		base = 1<<11;
	else if (x & 0x000F0000)
		base = 1<<9;
	else
		base = 1<<7;

    y = 0;
    while (base) {
		y += base;
		if  ((y * y) > x)
			y -= base;
		base >>= 1;
    }
    return y;
}


/*
	icexp - fixed point complex exponential
		phase - 0 to 255 representing 0 to 2pi
		return cos and sin in 1p10 format
*/
static void icexp(short c[2], int phase)
{
	/* cosine table generated with Matlab: round(1024*cos(2*pi*[0:255]/256) */
	static short cosTable[256] = {
		1024, 1024, 1023, 1021, 1019, 1016, 1013, 1009,
		1004, 999, 993, 987, 980, 972, 964, 955,
		946, 936, 926, 915, 903, 891, 878, 865,
		851, 837, 822, 807, 792, 775, 759, 742,
		724, 706, 688, 669, 650, 630, 610, 590,
		569, 548, 526, 505, 483, 460, 438, 415,
		392, 369, 345, 321, 297, 273, 249, 224,
		200, 175, 150, 125, 100, 75, 50, 25,
		0, -25, -50, -75, -100, -125, -150, -175,
		-200, -224, -249, -273, -297, -321, -345, -369,
		-392, -415, -438, -460, -483, -505, -526, -548,
		-569, -590, -610, -630, -650, -669, -688, -706,
		-724, -742, -759, -775, -792, -807, -822, -837,
		-851, -865, -878, -891, -903, -915, -926, -936,
		-946, -955, -964, -972, -980, -987, -993, -999,
		-1004, -1009, -1013, -1016, -1019, -1021, -1023, -1024,
		-1024, -1024, -1023, -1021, -1019, -1016, -1013, -1009,
		-1004, -999, -993, -987, -980, -972, -964, -955,
		-946, -936, -926, -915, -903, -891, -878, -865,
		-851, -837, -822, -807, -792, -775, -759, -742,
		-724, -706, -688, -669, -650, -630, -610, -590,
		-569, -548, -526, -505, -483, -460, -438, -415,
		-392, -369, -345, -321, -297, -273, -249, -224,
		-200, -175, -150, -125, -100, -75, -50, -25,
		0, 25, 50, 75, 100, 125, 150, 175,
		200, 224, 249, 273, 297, 321, 345, 369,
		392, 415, 438, 460, 483, 505, 526, 548,
		569, 590, 610, 630, 650, 669, 688, 706,
		724, 742, 759, 775, 792, 807, 822, 837,
		851, 865, 878, 891, 903, 915, 926, 936,
		946, 955, 964, 972, 980, 987, 993, 999,
		1004, 1009, 1013, 1016, 1019, 1021, 1023, 1024};
	c[0] = cosTable[phase & 0xFF];
	c[1] = cosTable[(phase-64) & 0xFF];
}


/*
	icMult - fixed point complex multiply
		r = a*b
*/
static void icMult(int32_t r[2], int32_t a[2], int32_t b0, int32_t b1)
{
	int32_t t;
	t = a[0]*b0 - a[1]*b1;
	r[1] = a[0]*b1 + a[1]*b0;
	r[0] = t;
}


/*
	------------ DIVIDER AND LNA CALIBRATION --------
*/
typedef	struct {
	LONG	i:8;
	LONG	q:8;
}	COMPLEX_VALUE;		/* Signed 8-bit complex values */

#define CALC_LENGTH		1024				/* Number of samples used to perform phase calculation for LNA or Divider Calibration */
#define CALC_LENGTH_DC	(CALC_LENGTH+512)	/* Number of samples used for DC removal */
#define MAX_CAPTURE_LENGTH		4096			/* Maximum number of samples to capture */
#define DIVCAL_CAPTURE_LENGTH	(CALC_LENGTH+1024)	/* Length of capture for Divider or LNA Calibration */

#define FIXED_M_PI		0x1000						/* Scaling for fixed point PI */
#define DEG(rad)		(radToDeg180(rad-FIXED_M_PI)+180)	/* Convert fixed radians (0x1000=pi) to degrees range [0 360) */
#define DEG180(rad)		radToDeg180(rad)		/* Convert fixed radians (0x1000=pi) to degrees range [-180 180) */

#define BYTE_PHASE_SHIFT		5					/* Shift to convert from byte phase (0x80=pi) to normal phase (0x1000=pi) */
#define CONVERT_TO_BYTE_PHASE(p)	(int)(((p)+(1<<(BYTE_PHASE_SHIFT-1)))>>BYTE_PHASE_SHIFT)	/* Convert from normal phase to byte phase */

#define R65_LNA_LOW		0x4
#define R65_LNA_MID		0x8
#define R65_LNA_HIGH	0xC


/*
	radMod2pi - converts angle in radians to the range [-pi pi)
*/
static LONG radMod2pi(LONG a)
{
	while (a < -FIXED_M_PI)
		a += 2*FIXED_M_PI;
	while (a >= FIXED_M_PI)
		a -= 2*FIXED_M_PI;

	return a;
}


/*
	radToDeg180 - converts angle in radians to the deg range [-180 180)
*/
static int radToDeg180(LONG rad)
{
	return (int)(radMod2pi(rad)*180/FIXED_M_PI);
}


/*
	avgPhase - computes the average phase.
		Phase is adjusted so all values are within the range mPhase[0] +/-pi
			mPhase - values to average (radians)
			pLength - number of values to average
		return average
*/
static LONG avgPhase(LONG mPhase[], int pLength)
{
	int i;
	short cval[2];
	LONG sumCos = 0, sumSin=0;
	for (i=0; i<pLength; i++) {
		icexp(cval, CONVERT_TO_BYTE_PHASE(mPhase[i]));
		sumCos += cval[0];
		sumSin += cval[1];
	}

	return iAtan2(sumSin, sumCos);
}


typedef
	COMPLEX_VALUE (*PCAP_IQ_DATA)[3];	/* CAP_IQ_DATA - Buffer to hold I/Q data for three RX chains */


/*
	RemoveDC - calculate mean and subtract. Return peak values
		peak - used to return the peak value of the three RX chains
		iqData - pointer to array of I/Q data for the three RX chains. DC is removed from the samples
		dataLength - number of samples in iqData
*/
static void RemoveDC(
	IN int peak[3],
	IN COMPLEX_VALUE (*iqData)[3],
	IN int dataLength)
{
	int i, j;
	int dcI[3] = {0, 0, 0};
	int dcQ[3] = {0, 0, 0};

	/* Calculate DC offset for each RX chain */
	for (i=0; i<dataLength; i++) {
		for (j=0; j<3; j++) {
			dcI[j] += iqData[i][j].i;
			dcQ[j] += iqData[i][j].q;
		}
	}

	for (j=0; j<3; j++) {
		dcI[j] /= dataLength;
		dcQ[j] /= dataLength;
	}

	/* Subtract DC and find peak */
	peak[0] = peak[1] = peak[2] = 0;

	for (i=0; i<dataLength; i++) {
		for (j=0; j<3; j++) {
			int sati = iqData[i][j].i - dcI[j];
			int satq = iqData[i][j].q - dcQ[j];

			/* Saturate */
			if (sati > 127)
				sati = 127;
			else if (sati < -128)
				sati = -128;
			iqData[i][j].i = sati;

			if (satq > 127)
				satq = 127;
			else if (satq < -128)
				satq = -128;
			iqData[i][j].q = satq;

			/* Record peak */
			if (peak[j] < iqData[i][j].i)
				peak[j] = iqData[i][j].i;
			if (peak[j] < iqData[i][j].q)
				peak[j] = iqData[i][j].q;
		}
	}
}


/*
	CalcRFCalPhase - process RF calibration to calculate phase of the three channels
		Parameters:
			phase - returns the phase of each channel. Fixed point value scaled so 0x1000 = PI
			avgI, avgQ - returns the avg I/Q of each channel. Implied scale factor of 256
			peak - returns the peak value of each channel after DC removal
			iqData - the input I/Q data for three channels. DC is removed.
			relPhase - If true it returns phase relative to Ant1. Otherwise it returns the
						phase relative to the reference signal.
			actTx - index of an active TX chain, used to detect start of signal
*/
static void CalcRFCalPhase(
	OUT LONG phase[3],
	OUT int avgI[3],
	OUT int avgQ[3],
	OUT int peak[3],
	IN COMPLEX_VALUE (*iqData)[3],
	IN BOOLEAN relPhase,
	IN int actTx)
{
	int i, j;
	LONG sumI[3], sumQ[3];
	static CHAR refSignal[64] = {	/* round(sin(-[0:63]*6*pi/64)*127) - three cycles per 64 samples */
		0, -37, -71, -98, -117, -126, -125, -112,
		-90, -60, -25, 12, 49, 81, 106, 122,
		127, 122, 106, 81, 49, 12, -25, -60,
		-90, -112, -125, -126, -117, -98, -71, -37,
		0, 37, 71, 98, 117, 126, 125, 112,
		90, 60, 25, -12, -49, -81, -106, -122,
		-127, -122, -106, -81, -49, -12, 25, 60,
		90, 112, 125, 126, 117, 98, 71, 37};


	/* Skip the first 200 samples to avoid the transient at the beginning */
	iqData += 200;

	/* Remove DC offset to help with low signal levels */
	RemoveDC(peak, iqData, CALC_LENGTH_DC);

	/* Search active channel to find sample with abs>12 */
	for (i=0; i<(CALC_LENGTH_DC-CALC_LENGTH); i++, iqData++) {
		if ((iqData[0][actTx].i*iqData[0][actTx].i + iqData[0][actTx].q*iqData[0][actTx].q) >= 144)
			break;
	}

	/* Move in 16 samples */
	iqData += 16;

	/* Sum the I and Q then calculate the angle of the sum */
	sumI[0] = sumI[1] = sumI[2] = 0;
	sumQ[0] = sumQ[1] = sumQ[2] = 0;

	for (i=0; i<CALC_LENGTH; i++) {
		/* Either calculate the phase relative to Ant1 or phase relative to reference */
		if (relPhase) {
			sumQ[0] += -iqData[i][0].i*iqData[i][1].q + iqData[i][0].q*iqData[i][1].i;
			sumI[0] +=  iqData[i][0].i*iqData[i][1].i + iqData[i][0].q*iqData[i][1].q;
			sumQ[2] += -iqData[i][2].i*iqData[i][1].q + iqData[i][2].q*iqData[i][1].i;
			sumI[2] +=  iqData[i][2].i*iqData[i][1].i + iqData[i][2].q*iqData[i][1].q;
		}
		else {
			int cval= refSignal[(i+16) % 64];
			int sval= refSignal[i % 64];
			for (j=0; j<3; j++) {
				sumQ[j] += -iqData[i][j].i*sval + iqData[i][j].q*cval;
				sumI[j] +=  iqData[i][j].i*cval + iqData[i][j].q*sval;
			}
		}
	}


	for (i=0; i<3; i++) {
		if (relPhase && i==1) {
			phase[i] = 0;
			avgI[i] = avgQ[i] = 0;
		}
		else {
		phase[i] = iAtan2(sumQ[i]>>6, sumI[i]>>6);
			/* Multiplication by refSignal added a scale factor of 128. Shift left by 1 for 256 scale factor */
			avgI[i] = (sumI[i]<<1)/CALC_LENGTH;
			avgQ[i] = (sumQ[i]<<1)/CALC_LENGTH;
		}
	}
}




#ifdef DBG
#ifdef LINUX
/* #define TIMESTAMP_CAL_CAPTURE0 */
/* #define TIMESTAMP_CAL_CAPTURE1 */
#endif /* LINUX */
#endif /* DBG */


/*
	mapChannelKHz - map channel number to KHz
*/
static LONG mapChannelKHz(int ch)
{
	long x;
	MAP_CHANNEL_ID_TO_KHZ(ch, x);
	return x;
}


/*
	InterpParam - Interpolate calibration parameters
		ch - channel to interpolate for
		chBeg, chEnd - begining and ending channel
		yBeg, yEnd - the hex phase values corresponding to chBeg and chEnd
*/
#ifdef MT76x2
static UCHAR InterpParam_check(int ch, int chBeg, int chEnd, UCHAR yBeg, UCHAR yEnd)
{
	long x, xBeg, xEnd, yDelta;
	UCHAR output;

	x = mapChannelKHz(ch);
	xBeg = mapChannelKHz(chBeg);
	xEnd = mapChannelKHz(chEnd);
	yDelta = yEnd - yBeg;

	/*
		Handle the phase wraparound. We always assume the delta phase is in
		the range [-180, 180] degrees = [0x80, 0x7f] in hex
	*/
	if (yDelta >= 128)
		yDelta -= 256;
	else if (yDelta <= -128)
		yDelta += 256;

	output = yBeg + yDelta*(x-xBeg)/(xEnd-xBeg);

	DBGPRINT(RT_DEBUG_TRACE,
		  ("%s : \n"
		   "x    = mapChannelKHz(%d) = %ld\n"
		   "xBeg = mapChannelKHz(%d) = %ld\n"
		   "xEnd = mapChannelKHz(%d) = %ld\n"
		   "yDelta = %ld\n"
		   "output = %d\n",
		   __FUNCTION__,
		   ch, x,
		   chBeg, xBeg,
		   chEnd, xEnd,
		   yDelta,
		   output));

	return output;
}
#endif


/*
	InterpParam - Interpolate calibration parameters
		ch - channel to interpolate for
		chBeg, chEnd - begining and ending channel
		yBeg, yEnd - the hex phase values corresponding to chBeg and chEnd
*/
static UCHAR InterpParam(int ch, int chBeg, int chEnd, UCHAR yBeg, UCHAR yEnd)
{
	long x, xBeg, xEnd, yDelta;

	x = mapChannelKHz(ch);
	xBeg = mapChannelKHz(chBeg);
	xEnd = mapChannelKHz(chEnd);
	yDelta = yEnd - yBeg;

	/*
		Handle the phase wraparound. We always assume the delta phase is in
		the range [-180, 180] degrees = [0x80, 0x7f] in hex
	*/
	if (yDelta >= 128)
		yDelta -= 256;
	else if (yDelta <= -128)
		yDelta += 256;

	return yBeg + yDelta*(x-xBeg)/(xEnd-xBeg);
}


/*
	ITxBFDivParams - interpolate Divider calibration parameter based on channel and EEPROM
		divValues - returns the Divider Calibration values for this channel
		channel - the channel to interpolate for
		divParams - the Divider Calibration parameters from EEPROM
*/

#ifdef MT76x2
static void mt76x2_ITxBFDivParams(UCHAR divValues[2], int channel, ITXBF_DIV_PARAMS *divParams)
{
	if (channel <= 14)
		divValues[0] = InterpParam(channel, 1, 14, divParams->E1gBeg, divParams->E1gEnd);
	else if (channel <= 60)
		divValues[0] = InterpParam(channel, 36, 64, divParams->E1aLowBeg, divParams->E1aLowEnd);
	//else if (channel <= 60)
	//	divValues[0] = InterpParam(channel, 44, 60, divParams->aLowBeg, divParams->aLowEnd);
	else if (channel <= 120)
		divValues[0] = InterpParam(channel, 100, 120, divParams->E1aMidBeg, divParams->E1aMidMid);
	else if (channel <= 140)
		divValues[0] = InterpParam(channel, 120, 140, divParams->E1aMidMid, divParams->E1aMidEnd);
	else
		divValues[0] = InterpParam(channel, 149, 173, divParams->E1aHighBeg, divParams->E1aHighEnd);
}
#endif

/*
	ITxBFLnaParams - interpolate LNA compensation parameter based on channel and EEPROM.
		lnaValues - returns the quantized LNA compensation values for M-L, H-L and H-M
		channel - the channel to interpolate for
		lnaParams - the LNA Calibration parameters from EEPROM
*/

#ifdef MT76x2
static void mt76x2_ITxBFLnaParams(UCHAR lnaValues[3], int channel, ITXBF_LNA_PARAMS *lnaParams)
{
	int i;

	if (channel <= 14) {
		lnaValues[0] = InterpParam(channel, 1, 14, lnaParams->E1gBeg[0], lnaParams->E1gEnd[0]);
		lnaValues[1] = InterpParam(channel, 1, 14, lnaParams->E1gBeg[1], lnaParams->E1gEnd[1]);
		lnaValues[2] = InterpParam(channel, 1, 14, lnaParams->E1gBeg[2], lnaParams->E1gEnd[2]);
	}
	else if (channel <= 64) {
		lnaValues[0] = InterpParam_check(channel, 36, 64, lnaParams->E1aLowBeg[0], lnaParams->E1aLowEnd[0]);
		lnaValues[1] = InterpParam_check(channel, 36, 64, lnaParams->E1aLowBeg[1], lnaParams->E1aLowEnd[1]);
		lnaValues[2] = InterpParam_check(channel, 36, 64, lnaParams->E1aLowBeg[2], lnaParams->E1aLowEnd[2]);
	}
	else if (channel <= 120) {
		lnaValues[0] = InterpParam(channel, 100, 120, lnaParams->E1aMidBeg[0], lnaParams->E1aMidMid[0]);
		lnaValues[1] = InterpParam(channel, 100, 120, lnaParams->E1aMidBeg[1], lnaParams->E1aMidMid[1]);
		lnaValues[2] = InterpParam(channel, 100, 120, lnaParams->E1aMidBeg[2], lnaParams->E1aMidMid[2]);
	}
		else if (channel <= 140) {
		lnaValues[0] = InterpParam(channel, 120, 140, lnaParams->E1aMidMid[0], lnaParams->E1aMidEnd[0]);
		lnaValues[1] = InterpParam(channel, 120, 140, lnaParams->E1aMidMid[1], lnaParams->E1aMidEnd[1]);
		lnaValues[2] = InterpParam(channel, 120, 140, lnaParams->E1aMidMid[2], lnaParams->E1aMidEnd[2]);
	}
	else {
		lnaValues[0] = InterpParam(channel, 149, 173, lnaParams->E1aHighBeg[0], lnaParams->E1aHighEnd[0]);
		lnaValues[1] = InterpParam(channel, 149, 173, lnaParams->E1aHighBeg[1], lnaParams->E1aHighEnd[1]);
		lnaValues[2] = InterpParam(channel, 149, 173, lnaParams->E1aHighBeg[2], lnaParams->E1aHighEnd[2]);
	}
}
#endif


/*
	ITxBFPhaseParams - interpolate Phase compensation parameters based on channel and EEPROM
		phaseValues - returns the Phase compensation values for this channel
		channel - the channel to interpolate for
		phaseParams - the Phase Calibration parameters from EEPROM
*/

#ifdef MT76x2
static void mt76x2_ITxBFPhaseParams(UCHAR phaseValues[2], int channel, ITXBF_PHASE_PARAMS *phaseParams)
{
	if (channel <= 14) {
		phaseValues[0] = InterpParam(channel, 1, 14, phaseParams->E1gBeg, phaseParams->E1gEnd);
	}
	else if (channel <= 64) {
		phaseValues[0] = InterpParam(channel, 36, 64, phaseParams->E1aLowBeg, phaseParams->E1aLowEnd);
	}
	else if (channel <= 120) {
		phaseValues[0] = InterpParam(channel, 100, 120, phaseParams->E1aMidBeg, phaseParams->E1aMidMid);
	}
	else if (channel <= 140) {
		phaseValues[0] = InterpParam(channel, 120, 140, phaseParams->E1aMidMid, phaseParams->E1aMidEnd);
	}
	else {
		phaseValues[0] = InterpParam(channel, 149, 173, phaseParams->E1aHighBeg, phaseParams->E1aHighEnd);
	}
}
#endif


#define ITXBF_EEPROM_WORDS		19	/* 38 bytes of ITxBF parameters */


/*
	ITxBFLoadLNAComp - load the LNA compensation registers
*/

/*
	ITxBFDividerCalibration - perform divider calibration
		calFunction - the function to perform
			0=>Display cal param,
			1=>Update EEPROM
			2=>Update BBP
			3=>Just return the quantized divider phase in divPhase
			10=> Display params and dump capture data
		calMethod - the calibration method to use. 0=>use default method for the band
		divPhase - if not NULL, returns the quantized divider phase (0, +/-90, 180 for 2.4G, 0,180 for 5G)
	returns TRUE if no errors
*/
#define ITXBF_MAX_WAIT_CYCLE	10000

#define MAX_LNA_CAPS	10		/* Maximum number of LNA captures per calibration */



#ifdef MT76x2

INT    avgPhase32[3];
UCHAR  MidVGA[2];

#endif // MT76x2

#ifdef MT76x2
static SC_TABLE_ENTRY impSubCarrierTable[3] = { {224, 255, 1, 31}, {198, 254, 2, 58}, {134, 254, 2, 122} };
static SC_TABLE_ENTRY expSubCarrierTable[3] = { {224, 255, 1, 31}, {198, 254, 2, 58}, {134, 254, 2, 122} };

#endif /* MT76x2 */

#ifdef DBG
#ifdef LINUX
/* #define TIMESTAMP_CALC_CALIBRATION */
#endif /* LINUX */
#endif /* DBG */

#define P_RESOLUTION	256		/* Resolution of phase calculation: 2pi/256 */

static UCHAR maxCarrTab[3] = {PROFILE_MAX_CARRIERS_20,
		                      PROFILE_MAX_CARRIERS_40,
		                 	  PROFILE_MAX_CARRIERS_80};

int32_t ei0[PROFILE_MAX_CARRIERS_40][2];
int32_t ei1[PROFILE_MAX_CARRIERS_40][2];
int32_t ei2[PROFILE_MAX_CARRIERS_40][2];

