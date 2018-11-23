#ifndef _FORMAT_H_
#define _FORMAT_H_

#include "stdafx.h"

const char report_interval[] =   "           %4.1lf-%4.1lf sec  %ss  %ss/sec\n";
const char result_t_upload[] =   "[T-Upload] %4.1lf-%4.1lf sec  %ss  %ss/sec\n";
const char result_t_download[] = "[T-Dwload] %4.1lf-%4.1lf sec  %ss  %ss/sec\n";
const char result_n_upload[] =   "[N-Upload] %4.1lf-%4.1lf sec  %ss  %ss/sec\n";
const char result_n_download[] = "[N-Dwload] %4.1lf-%4.1lf sec  %ss  %ss/sec\n";

const long kKilo_to_Unit = 1024;
const long kMega_to_Unit = 1024 * 1024;
const long kGiga_to_Unit = 1024 * 1024 * 1024;

const long kkilo_to_Unit = 1000;
const long kmega_to_Unit = 1000 * 1000;
const long kgiga_to_Unit = 1000 * 1000 * 1000;

enum {
	kConv_Unit,
	kConv_Kilo,
	kConv_Mega,
	kConv_Giga
};

const double kConversion[] =
{
	1.0,                       /* unit */
	1.0 / 1024,                /* Kilo */
	1.0 / 1024 / 1024,         /* Mega */
	1.0 / 1024 / 1024 / 1024   /* Giga */
};

const double kConversionForBits[] =
{
	1.0,                       /* unit */
	1.0 / 1000,                /* kilo */
	1.0 / 1000 / 1000,         /* mega */
	1.0 / 1000 / 1000 / 1000   /* giga */
};

/* labels for Byte formats [KMG] */
const char* kLabel_Byte[] =
{
	"Byte",
	"KByte",
	"MByte",
	"GByte"
};

/* labels for bit formats [kmg] */
const char* kLabel_bit[]  =
{
	"bit", 
	"Kbit",
	"Mbit",
	"Gbit"
};

void ByteSprintf(char * outString, int inLen, double inNum, char inFormat)
{
	int conv;
	const char* suffix;
	const char* format;
	if(!isupper((int)inFormat))
		inNum *= 8; //bit
	switch(toupper(inFormat))
	{
	case 'B': conv = kConv_Unit; break;
	case 'K': conv = kConv_Kilo; break;
	case 'M': conv = kConv_Mega; break;
	case 'G': conv = kConv_Giga; break;
	default:
	case 'A':
		{
			double tmpNum = inNum;
			conv = kConv_Unit;
			if(isupper((int)inFormat)){
				while(tmpNum >= 1024.0 && conv <= kConv_Giga){
					tmpNum /= 1024.0;
					conv++;
				}
			}else{
				while(tmpNum >= 1000.0 && conv <= kConv_Giga) {
					tmpNum /= 1000.0;
					conv++;
				}
			}
			break;
		}
	}
	if(!isupper((int)inFormat)) {
		inNum *= kConversionForBits[conv];
		suffix = kLabel_bit[conv];
	}
	else {
		inNum *= kConversion[conv];
		suffix = kLabel_Byte[conv];
	}

	if(inNum < 9.995) {
		format = "%4.2f %s";
	}
	else if (inNum < 99.95) {
		format = "%4.1f %s";
	}
	else if(inNum < 999.5) {
		format = "%4.0f %s";
	}
	else {
		format = "%4.0f %s";
	}
	_snprintf_s(outString, inLen, inLen, format, inNum, suffix);
}

#endif //_FORMAT_H_