#include <stdlib.h>

extern "C" {
#include <SDL.h>
}

#include "codec/AVAudioReader.h"
#include "codec/AVAudioFrame.h"
#include "LogUtil.h"

using namespace avfun::codec;

static Uint32 audio_len;
static Uint8* audio_pos;

/* The audio function callback takes the following parameters:
   stream:  A pointer to the audio buffer to be filled
   len:     The length (in bytes) of the audio buffer
*/
void fill_audio(void* udata, Uint8* stream, int len)
{
	/* Only play if we have data left */
	SDL_memset(stream, 0, len);
	if (audio_len == 0)
		return;

	/* Mix as much data as possible */
	len = (len > audio_len ? audio_len : len);
	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len;
	audio_len -= len;
}

int openAudioDevice() {
	SDL_AudioSpec wanted;

	/* Set the audio format */
	wanted.freq = 44100;
	wanted.format = AUDIO_S16;
	wanted.channels = 1;    /* 1 = mono, 2 = stereo */
	wanted.samples = 1024;  /* Good low-latency value for callback */
	wanted.callback = fill_audio;
	wanted.userdata = NULL;

	/* Open the audio device, forcing the desired format */
	if (SDL_OpenAudio(&wanted, NULL) < 0) {
		fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
		return(-1);
	}
	return(0);
}



int main(int argc,char* argv[]) {

	openAudioDevice();

	/* Load the audio data ... */

	constexpr auto video_filename = "C:/hecc/develop/github/avfun/avfun/example/resource/audio01.mp3";

	auto ar = AVAudioReader::Make(video_filename);
	ar->SetupDecoder();

	//audio_pos = audio_chunk;

	/* Let the callback function play the audio chunk */
	SDL_PauseAudio(0);

	/* Do some processing */

	while (true) {
		auto aframe = ar->ReadNextFrame();
		if (aframe == nullptr) break;
		audio_pos = aframe->get()[0];
		audio_len = aframe->GetSize();

		/* Wait for sound to complete */
		while (audio_len > 0) {
			SDL_Delay(1);         /* Sleep 1/10 second */
		}
	}



	SDL_CloseAudio();






	return 0;
}