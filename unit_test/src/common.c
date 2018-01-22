//*****************************************************************
// Copyright 2018 NXP
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//*****************************************************************

#include "common.h"


void xa_shift_input_buffer (char *buf, int buf_size, int bytes_consumed)
{
	int i;

	if(bytes_consumed <= 0)
	    return;

	for (i = 0; i < buf_size - bytes_consumed; i++)
	{
	    buf[i] = buf[i + bytes_consumed];
	}
}


/*******************************************************************************
 *
 *   FUNCTION NAME - App_get_mp4forbsac_header
 *
 *   DESCRIPTION
 *       Gets mp4forbsac header from the input bitstream.
 *
 *   ARGUMENTS
 *         params  -  place to store the mp4forbsac-header data
 *
 *   RETURN VALUE
 *         Success :  0
 *         Error   : -1
*******************************************************************************/
int App_get_mp4forbsac_header(BSAC_Block_Params * params, char *buf, int buf_size)
{

    int byte1,byte2,byte3,byte4;

    if(buf_size <= 16)
        return -1;

    byte1 = buf[0];
    byte2 = buf[1];
    byte3 = buf[2];
    byte4 = buf[3];
    params->scalOutObjectType = byte1 + (byte2 <<8) + (byte3 <<16) + (byte4 <<24);

    byte1 = buf[4];
    byte2 = buf[5];
    byte3 = buf[6];
    byte4 = buf[7];
    params->scalOutNumChannels = byte1 + (byte2 <<8) + (byte3 <<16) + (byte4 <<24);

    byte1 = buf[8];
    byte2 = buf[9];
    byte3 = buf[10];
    byte4 = buf[11];
    params->sampleRate = byte1 + (byte2 <<8) + (byte3 <<16) + (byte4 <<24);


    if(params->sampleRate == 96000) params->SamplingFreqIndex = 0;
    else if( params->sampleRate == 88200) params->SamplingFreqIndex = 1;
    else if( params->sampleRate == 64000) params->SamplingFreqIndex = 2;
    else if( params->sampleRate == 48000) params->SamplingFreqIndex = 3;
    else if( params->sampleRate == 44100) params->SamplingFreqIndex = 4;
    else if( params->sampleRate == 32000) params->SamplingFreqIndex = 5;
    else if( params->sampleRate == 24000) params->SamplingFreqIndex = 6;
    else if( params->sampleRate == 22050) params->SamplingFreqIndex = 7;
    else if( params->sampleRate == 16000) params->SamplingFreqIndex = 8;
    else if( params->sampleRate == 12000) params->SamplingFreqIndex = 9;
    else if( params->sampleRate == 11025) params->SamplingFreqIndex = 10;
    else if( params->sampleRate ==  8000) params->SamplingFreqIndex = 11;
    else if( params->sampleRate ==  7350) params->SamplingFreqIndex = 12;
    else params->SamplingFreqIndex = 13;




    byte1 = buf[12];
    byte2 = buf[13];
    byte3 = buf[14];
    byte4 = buf[15];
    params->framelengthflag = byte1 + (byte2 <<8) + (byte3 <<16) + (byte4 <<24);


    return 0;
}

