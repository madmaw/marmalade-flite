
/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2001                            */
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
/*  Simple top level program                                             */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "flite.h"
#include "cst_wave.h"
//#include "flite_version.h"
#include "voxdefs.h"
#include <s3eSound.h>
#include <s3eAudio.h>
#include <s3eDevice.h>

void cst_alloc_debug_summary();

cst_voice *REGISTER_VOX(const char *voxdir);
cst_voice *UNREGISTER_VOX(cst_voice *vox);
/*
static void flite_version()
{
    printf("  version: %s-%s-%s %s\n",
           FLITE_PROJECT_PREFIX,
           FLITE_PROJECT_VERSION,
           FLITE_PROJECT_STATE,
           FLITE_PROJECT_DATE);
    printf("  CMU Copyright 1999-2005, all rights reserved\n");
}
*/


static void flite_usage()
{
    printf("flite: a small simple speech synthesizer\n");
    //flite_version();
    printf("usage: flite TEXT/FILE [WAVEFILE]\n"
           "  Converts text in TEXTFILE to a waveform in WAVEFILE\n"
           "  If text contains a space the it is treated as a literal\n"
           "  textstring and spoken, and not as a file name\n"
           "  if WAVEFILE is unspecified or \"play\" the result is\n"
           "  played on the current systems audio device.  If WAVEFILE\n"
           "  is \"none\" the waveform is discarded (good for benchmarking)\n"
           "  Other options must appear before these options\n"
           //"  --version   Output flite version number\n"
           "  --help      Output usage string\n"
           "  -o WAVEFILE Explicitly set output filename\n"
           "  -f TEXTFILE Explicitly set input filename\n"
           "  -t TEXT     Explicitly set input textstring\n"
           "  -p PHONES   Explicitly set input textstring and synthesize as phones\n"
           "  --set F=V   Set feature (guesses type)\n"
           "  -s F=V      Set feature (guesses type)\n"
           "  --seti F=V  Set int feature\n"
           "  --setf F=V  Set float feature\n"
           "  --sets F=V  Set string feature\n"
           "  -b          Benchmark mode\n"
           "  -l          Loop endlessly\n"
           "  -pw         Print words\n"
           "  -ps         Print segments\n"
           "  -pr RelName  Print relation RelName\n"
           "  -v          Verbose mode\n");
    exit(0);
}

static cst_utterance *print_info(cst_utterance *u)
{
    cst_item *item;
    const char *relname;

    relname = utt_feat_string(u,"print_info_relation");
    for (item=relation_head(utt_relation(u,relname)); 
         item; 
         item=item_next(item))
    {
        printf("%s ",item_feat_string(item,"name"));
    }
    printf("\n");

    return u;
}

static void ef_set(cst_features *f,const char *fv,const char *type)
{
    /* set feature from fv (F=V), guesses type if not explicit type given */
    const char *val;
    char *feat;

    if ((val = strchr(fv,'=')) == 0)
    {
        fprintf(stderr,
                "flite: can't find '=' in featval \"%s\", ignoring it\n",
                fv);
    }
    else
    {
        feat = cst_strdup(fv);
        feat[strlen(fv)-strlen(val)] = '\0';
        val = val+1;
        if ((type && cst_streq("int",type)) ||
            ((type == 0) && (cst_regex_match(cst_rx_int,val))))
            feat_set_int(f,feat,atoi(val));
        else if ((type && cst_streq("float",type)) ||
                 ((type == 0) && (cst_regex_match(cst_rx_double,val))))
            feat_set_float(f,feat,atof(val));
        else
            feat_set_string(f,feat,val);
        /* I don't free feat, because feats think featnames are const */
        /* which is true except in this particular case          */
    }
}

int flite_main(int argc, char **argv)
{
    struct timeval tv;
    cst_voice *v;
    const char *filename;
    const char *outtype;
    int i;
    float durs;
    double time_start, time_end;
    int flite_verbose, flite_loop, flite_bench;
    int explicit_filename, explicit_text, explicit_phones;
#define ITER_MAX 3
    int bench_iter = 0;
    cst_features *extra_feats;

    filename = 0;
    outtype = "play";   /* default is to play */
    flite_verbose = FALSE;
    flite_loop = FALSE;
    flite_bench = FALSE;
    explicit_text = explicit_filename = explicit_phones = FALSE;
    extra_feats = new_features();

    flite_init();

    for (i=1; i<argc; i++)
    {
        if (cst_streq(argv[i],"--version"))
        {
            //flite_version();
            return 1;
        }
        else if (cst_streq(argv[i],"-h") ||
                 cst_streq(argv[i],"--help") ||
                 cst_streq(argv[i],"-?"))
            flite_usage();
        else if (cst_streq(argv[i],"-v"))
            flite_verbose = TRUE;
        else if (cst_streq(argv[i],"-l"))
            flite_loop = TRUE;
        else if (cst_streq(argv[i],"-b"))
        {
            flite_bench = TRUE;
            break; /* ignore other arguments */
        }
        else if ((cst_streq(argv[i],"-o")) && (i+1 < argc))
        {
            outtype = argv[i+1];
            i++;
        }
        else if (cst_streq(argv[i],"-f") && (i+1 < argc))
        {
            filename = argv[i+1];
            explicit_filename = TRUE;
            i++;
        }
        else if (cst_streq(argv[i],"-pw"))
        {
            feat_set_string(extra_feats,"print_info_relation","Word");
            feat_set(extra_feats,"post_synth_hook_func",
                     uttfunc_val(&print_info));
        }
        else if (cst_streq(argv[i],"-ps"))
        {
            feat_set_string(extra_feats,"print_info_relation","Segment");
            feat_set(extra_feats,"post_synth_hook_func",
                     uttfunc_val(&print_info));
        }
        else if (cst_streq(argv[i],"-pr") && (i+1 < argc))
        {
            feat_set_string(extra_feats,"print_info_relation",argv[i+1]);
            feat_set(extra_feats,"post_synth_hook_func",
                     uttfunc_val(&print_info));
            i++;
        }
        else if ((cst_streq(argv[i],"-set") || cst_streq(argv[i],"-s"))
                 && (i+1 < argc))
        {
            ef_set(extra_feats,argv[i+1],0);
            i++;
        }
        else if (cst_streq(argv[i],"--seti") && (i+1 < argc))
        {
            ef_set(extra_feats,argv[i+1],"int");
            i++;
        }
        else if (cst_streq(argv[i],"--setf") && (i+1 < argc))
        {
            ef_set(extra_feats,argv[i+1],"float");
            i++;
        }
        else if (cst_streq(argv[i],"--sets") && (i+1 < argc))
        {
            ef_set(extra_feats,argv[i+1],"string");
            i++;
        }
        else if (cst_streq(argv[i],"-p") && (i+1 < argc))
        {
            filename = argv[i+1];
            explicit_phones = TRUE;
            i++;
        }
        else if (cst_streq(argv[i],"-t") && (i+1 < argc))
        {
            filename = argv[i+1];
            explicit_text = TRUE;
            i++;
        }
        else if (filename)
            outtype = argv[i];
        else
            filename = argv[i];
    }

    if (filename == NULL) filename = "-";  /* stdin */
    v = REGISTER_VOX(NULL);
    feat_copy_into(extra_feats,v->features);
    durs = 0.0;

    if (flite_bench)
    {
        outtype = "none";
        filename = "A whole joy was reaping, but they've gone south, you should fetch azure mike.";
        explicit_text = TRUE;
    }

loop:
    gettimeofday(&tv,NULL);
    time_start = (double)(tv.tv_sec)+(((double)tv.tv_usec)/1000000.0);

    if (explicit_phones)
        durs = flite_phones_to_speech(filename,v,outtype);
    else if ((strchr(filename,' ') && !explicit_filename) || explicit_text)
        durs = flite_text_to_speech(filename,v,outtype);
        // commented out for iPhone:
    //else
        //durs = flite_file_to_speech(filename,v,outtype);

    gettimeofday(&tv,NULL);
    time_end = ((double)(tv.tv_sec))+((double)tv.tv_usec/1000000.0);

    if (flite_verbose || (flite_bench && bench_iter == ITER_MAX))
        printf("times faster than real-time: %f\n(%f seconds of speech synthesized in %f)\n",
               durs/(float)(time_end-time_start),
               durs,
               (float)(time_end-time_start));

    if (flite_loop || (flite_bench && bench_iter++ < ITER_MAX))
            goto loop;

    delete_features(extra_feats);
    UNREGISTER_VOX(v);

    return 0;
}

int g_closed;
int g_channel;

int32 test_close(void *systemData, void *userData) {
	g_closed = 1;
}

int32 test_wave(void *systemData, void *userData) {
	cst_wave *wave;
	cst_voice *voice = (cst_voice*)userData;
	wave = flite_text_to_wave("wait for it, wait for it, hello world, hello world", voice);
	s3eSoundChannelSetInt(g_channel, S3E_CHANNEL_RATE, wave->sample_rate);
	s3eSoundChannelSetInt(g_channel, S3E_CHANNEL_PITCH, 0x15000);
	s3eSoundChannelPlay(g_channel, (int16*)wave->samples, wave->num_samples, 2, 0);
	//s3eAudioPlayFromBuffer(wave->samples, wave->num_samples, 1);
}

int main() {
	/*
	int i, k;
	int pitch;
	int diff;
	cst_voice *voice;
	g_closed = 0;
	flite_init();
	s3eSoundResumeAllChannels();
	voice = REGISTER_VOX(NULL);
	
	//s3eSoundChannelRegister(g_channel, S3E_CHANNEL_GEN_AUDIO, test_wave, voice);
	//s3eSoundChannelRegister(g_channel, S3E_CHANNEL_STOP_AUDIO, test_close, NULL);
	//s3eSoundChannelPlay(channel, (int16*)wave->samples, wave->num_samples, 2, 0);
	test_wave(NULL, (void*)voice);
	while( !g_closed );
	UNREGISTER_VOX(voice);
	*/
	//flite_text_to_speech("hello world", voice, "play");
	/*
	FILE* file;
#define MAX_FILE_SIZE 200000
#define FILE_NAME "fat.wav"
	int n;	
	int i;
	int channel;
	char* argv[5];
	int16* buffer = (int16*)malloc(sizeof(int16)*MAX_FILE_SIZE);
	argv[0] = "flite";
	argv[1] = "-t";
	argv[2] = "kitty is a fat cat, a fat, fat kitty cat";
	argv[3] = "-o";
	argv[4] = FILE_NAME;
	flite_main( 5, argv );

	for( i=0; i<6; i++ ) {
		s3eDeviceYield(100);
	}
    //s3eSoundSetInt(S3E_SOUND_DEFAULT_FREQ, 16000);
	file = fopen(FILE_NAME, "rb");
	if( file ) {
		n = fread(buffer, sizeof(int16), MAX_FILE_SIZE, file);
		//s3eAudioPlayFromBuffer(buffer, n, 0);
		channel = s3eSoundGetFreeChannel();
		s3eSoundChannelSetInt(channel, S3E_CHANNEL_RATE, 16000);
		s3eSoundChannelPlay(channel, buffer, n, 2, 0);
		fclose(file);
	}
	*/
	char* argv[3];
	argv[0] = "flite";
	argv[1] = "-t";
	argv[2] = "fly my pretties! FLY!";
	flite_main( 3, argv );

	while( 1 ) {
		s3eDeviceYield(100);
	}
//	free(buffer);
	
}

