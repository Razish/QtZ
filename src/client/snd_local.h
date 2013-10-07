/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// snd_local.h -- private sound definations


#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "snd_public.h"

#define	PAINTBUFFER_SIZE		4096					// this is in samples

#define SND_CHUNK_SIZE			1024					// samples
#define SND_CHUNK_SIZE_FLOAT	(SND_CHUNK_SIZE/2)		// floats
#define SND_CHUNK_SIZE_BYTE		(SND_CHUNK_SIZE*2)		// floats

typedef struct portable_samplepair_s {
	int			left;	// the final values will be clamped to +/- 0x00ffff00 and shifted down
	int			right;
} portable_samplepair_t;

typedef struct adpcm_state_s {
    short	sample;		/* Previous output value */
    char	index;		/* Index into stepsize table */
} adpcm_state_t;

typedef	struct sndBuffer_s {
	short					sndChunk[SND_CHUNK_SIZE];
	struct sndBuffer_s		*next;
    int						size;
	adpcm_state_t			adpcm;
} sndBuffer_t;

typedef struct sfx_s {
	sndBuffer_t		*soundData;
	qboolean		defaultSound;			// couldn't be loaded, so use buzz
	qboolean		inMemory;				// not in Memory
	qboolean		soundCompressed;		// not in Memory
	int				soundCompressionMethod;	
	int 			soundLength;
	char 			soundName[MAX_QPATH];
	int				lastTimeUsed;
	struct sfx_s	*next;
} sfx_t;

typedef struct dma_s {
	int			channels;
	int			samples;				// mono samples in buffer
	int			submission_chunk;		// don't mix less than this #
	int			samplebits;
	int			speed;
	byte		*buffer;
} dma_t;

#define START_SAMPLE_IMMEDIATE	0x7fffffff

#define MAX_DOPPLER_SCALE 50.0f //arbitrary

#define THIRD_PERSON_THRESHOLD_SQ (48.0f*48.0f)

typedef struct loopSound_s {
	vector3		origin;
	vector3		velocity;
	sfx_t		*sfx;
	int			mergeFrame;
	qboolean	active;
	qboolean	kill;
	qboolean	doppler;
	float		dopplerScale;
	float		oldDopplerScale;
	int			framenum;
} loopSound_t;

typedef struct channel_s {
	int			allocTime;
	int			startSample;	// START_SAMPLE_IMMEDIATE = set immediately on next mix
	int			entnum;			// to allow overriding a specific sound
	int			entchannel;		// to allow overriding a specific sound
	int			leftvol;		// 0-255 volume after spatialization
	int			rightvol;		// 0-255 volume after spatialization
	int			master_vol;		// 0-255 volume before spatialization
	float		dopplerScale;
	float		oldDopplerScale;
	vector3		origin;			// only use if fixed_origin is set
	qboolean	fixed_origin;	// use origin instead of fetching entnum's origin
	sfx_t		*thesfx;		// sfx structure
	qboolean	doppler;
	qboolean	fullVolume;
} channel_t;


#define	WAV_FORMAT_PCM		1


typedef struct wavinfo_s {
	int		format;
	int		rate;
	int		width;
	int		channels;
	int		samples;
	int		dataofs;		// chunk starts this many bytes from file start
} wavinfo_t;

// Interface between Q3 sound "api" and the sound backend
typedef struct soundInterface_s {
	void (*Shutdown)( void );
	void (*StartSound)( vector3 *origin, int entnum, int entchannel, sfxHandle_t sfx );
	void (*StartLocalSound)( sfxHandle_t sfx, int channelNum );
	void (*StartBackgroundTrack)( const char *intro, const char *loop );
	void (*StopBackgroundTrack)( void );
	void (*RawSamples)(int stream, int samples, int rate, int width, int channels, const byte *data, float volume, int entityNum);
	void (*StopAllSounds)( void );
	void (*ClearLoopingSounds)( qboolean killall );
	void (*AddLoopingSound)( int entityNum, const vector3 *origin, const vector3 *velocity, sfxHandle_t sfx );
	void (*AddRealLoopingSound)( int entityNum, const vector3 *origin, const vector3 *velocity, sfxHandle_t sfx );
	void (*StopLoopingSound)(int entityNum );
	void (*Respatialize)( int entityNum, const vector3 *origin, matrix3 axis, int inwater );
	void (*UpdateEntityPosition)( int entityNum, const vector3 *origin );
	void (*Update)( void );
	void (*DisableSounds)( void );
	void (*BeginRegistration)( void );
	sfxHandle_t (*RegisterSound)( const char *sample, qboolean compressed );
	void (*ClearSoundBuffer)( void );
	void (*SoundInfo)( void );
	void (*SoundList)( void );
#ifdef USE_VOIP
	void (*StartCapture)( void );
	int (*AvailableCaptureSamples)( void );
	void (*Capture)( int samples, byte *data );
	void (*StopCapture)( void );
	void (*MasterGain)( float gain );
#endif
} soundInterface_t;


/*

	SYSTEM SPECIFIC FUNCTIONS

*/

// initializes cycling through a DMA buffer and returns information on it
qboolean SNDDMA_Init( void );

// gets the current DMA position
int		SNDDMA_GetDMAPos( void );

// shutdown the DMA xfer.
void	SNDDMA_Shutdown( void );

void	SNDDMA_BeginPainting( void );

void	SNDDMA_Submit( void );

#define	MAX_CHANNELS			96

extern	channel_t   s_channels[MAX_CHANNELS];
extern	channel_t   loop_channels[MAX_CHANNELS];
extern	int		numLoopChannels;

extern	int		s_paintedtime;
extern	vector3	listener_forward;
extern	vector3	listener_right;
extern	vector3	listener_up;
extern	dma_t	dma;

#define	MAX_RAW_SAMPLES	16384
#define MAX_RAW_STREAMS (MAX_CLIENTS * 2 + 1)
extern	portable_samplepair_t s_rawsamples[MAX_RAW_STREAMS][MAX_RAW_SAMPLES];
extern	int		s_rawend[MAX_RAW_STREAMS];

extern cvar_t *s_volume;
extern cvar_t *s_musicVolume;
extern cvar_t *s_muted;
extern cvar_t *s_doppler;

extern cvar_t *s_testsound;

qboolean S_LoadSound( sfx_t *sfx );

void		SND_free(sndBuffer_t *v);
sndBuffer_t	*SND_malloc( void );
void		SND_setup( void );
void		SND_shutdown( void );

void S_PaintChannels(int endtime);

void S_memoryLoad(sfx_t *sfx);

// spatializes a channel
void S_Spatialize(channel_t *ch);

// adpcm functions
int  S_AdpcmMemoryNeeded( const wavinfo_t *info );
void S_AdpcmEncodeSound( sfx_t *sfx, short *samples );
void S_AdpcmGetSamples(sndBuffer_t *chunk, short *to);

// wavelet function

#define SENTINEL_MULAW_ZERO_RUN 127
#define SENTINEL_MULAW_FOUR_BIT_RUN 126

void S_FreeOldestSound( void );

#define	NXStream byte

void encodeWavelet(sfx_t *sfx, short *packets);
void decodeWavelet( sndBuffer_t *stream, short *packets);

void encodeMuLaw( sfx_t *sfx, short *packets);
extern short mulawToShort[256];

extern short *sfxScratchBuffer;
extern sfx_t *sfxScratchPointer;
extern int	   sfxScratchIndex;

qboolean S_Base_Init( soundInterface_t *si );

// OpenAL stuff
typedef enum alSrcPriority_e {
	SRCPRI_AMBIENT = 0,	// Ambient sound effects
	SRCPRI_ENTITY,			// Entity sound effects
	SRCPRI_ONESHOT,			// One-shot sounds
	SRCPRI_LOCAL,				// Local sounds
	SRCPRI_STREAM				// Streams (music, cutscenes)
} alSrcPriority_t;

typedef int srcHandle_t;

qboolean S_AL_Init( soundInterface_t *si );
