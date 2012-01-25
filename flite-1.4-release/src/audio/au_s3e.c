/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2001                             */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  January 2001                                     */
/*************************************************************************/
/*                                                                       */
/*  s3e direct audio                                            */
/*                                                                       */
/*************************************************************************/

#include <s3eSound.h>
#include <s3eAudio.h>

#include "cst_string.h"
#include "cst_wave.h"
#include "cst_audio.h"

typedef struct {
	int channel; 
	int16 data[4096];
	int32 dataLength;
	int playing;
} s3e_audio_data;

int audio_buffer_s3e(void* systemData, void* userData) {
	s3e_audio_data* data = (s3e_audio_data*)userData;
	int result = data->dataLength;
	s3eSoundChannelPlay(data->channel, data->data, data->dataLength, 1, 0);
	data->dataLength = 0;
	return result;
}

cst_audiodev * audio_open_s3e(int sps, int channels, int fmt)
{
    cst_audiodev *ad;
	int channel;
	s3e_audio_data* data;
    ad = cst_alloc(cst_audiodev,1);
    ad->sps = sps;
    ad->channels = channels;
    ad->fmt = (cst_audiofmt)fmt;

	ad->real_sps = 16000;
	ad->real_channels = 1;
	ad->real_fmt = CST_AUDIO_LINEAR16;

	channel = s3eSoundGetFreeChannel();
	data = (s3e_audio_data*)malloc(sizeof(s3e_audio_data));
	data->channel = channel;
	data->dataLength = 0;
	data->playing = 1;
	s3eSoundChannelSetInt(channel, S3E_CHANNEL_RATE, 16000);
	//s3eSoundChannelSetInt(channel, S3E_CHANNEL_PITCH, 0x10000);	
	//s3eSoundChannelRegister(channel, S3E_CHANNEL_GEN_AUDIO, audio_buffer_s3e, data);
	ad->platform_data = (void*)data;
	
    return ad;
}

int audio_close_s3e(cst_audiodev *ad)
{
	s3e_audio_data* data;
	if (ad)
	{
		data = (s3e_audio_data*)ad->platform_data;
		
		s3eSoundChannelPlay(data->channel, data->data, data->dataLength, 1, 0);
		//s3eAudioPlayFromBuffer(data->data, data->dataLength, 1);
		//s3eSoundChannelStop(data->channel);
		// TODO free the stuff buffer
		//free(data);
		cst_free(ad);
	}
    return 0;
}

int audio_write_s3e(cst_audiodev *ad, void *samples, int num_bytes)
{
	int i, j;
	s3e_audio_data* data;
	data = (s3e_audio_data*)ad->platform_data;
	if( !data->playing ) {
		s3eSoundChannelPlay(data->channel, (int16*)samples, num_bytes, 1, 0);
		data->playing = 1;
	} else {
		for( i=0; i<num_bytes; i++ ) {
			data->data[data->dataLength] = ((int16*)samples)[i];
			data->dataLength++;
		}
	}
	(void)ad;
    (void)samples;
    return num_bytes;
}

int audio_drain_s3e(cst_audiodev *ad)
{
    (void)ad;
    return 0;
}

int audio_flush_s3e(cst_audiodev *ad)
{
    (void)ad;
    return 0;
}
