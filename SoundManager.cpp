
#include "SoundManager.h"
#include "util/Debug.h"

SoundManager::SoundManager()
{
    m_music = NULL;

    // Settings for audio device
    int audio_rate = 44100;			//Frequency of audio playback
    Uint16 audio_format = AUDIO_S16SYS; 	//Format of the audio we're playing
    int audio_channels = 2;			//2 channels = stereo
    int audio_buffers = 4096;		//Size of the audio buffers in memory

    //Initialize SDL audio
    if(SDL_Init(SDL_INIT_AUDIO) != 0)
    {
        LOG_ERROR(Logger::Misc, "Unable to initialize SDL: %s\n", SDL_GetError());
        return;
    }

    //Initialize SDL_mixer with our chosen audio settings
    if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0)
    {
        LOG_ERROR(Logger::Misc, "Unable to initialize audio: %s\n", Mix_GetError());
        return;
    }
}

SoundManager::~SoundManager()
{
    this->stop();

    Mix_CloseAudio();
    SDL_Quit();
}

/**
 * @brief SoundManager::play This method stops the currently playing music and starts the music given by filename
 * @param filename
 * @param loop
 */
void SoundManager::play(std::string filename, int loop)
{
    this->stop();

    m_music = Mix_LoadMUS(filename.c_str());
    if(m_music == NULL)
    {
        LOG_WARN(Logger::Misc, "Unable to load file: %s\n", Mix_GetError());
        return;
    }

    if( Mix_PlayMusic(m_music, loop) != 0 )
    {
        LOG_WARN(Logger::Misc, "Could not play music %s", filename.c_str());
    }
}

/**
 * @brief SoundManager::stop This method stops the currently playing sound (if any)
 */
void SoundManager::stop()
{
    if(m_music != NULL)
    {
        // Halt the music first and then free it
        Mix_HaltMusic();
        Mix_FreeMusic(m_music);

        m_music = NULL;
    }
}
