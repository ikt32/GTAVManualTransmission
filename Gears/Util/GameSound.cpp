#include "GameSound.h"
#include <inc/natives.h>
#include <utility>


GameSound::GameSound(std::string sound, std::string soundSet, std::string audioBank) :
    mAudioBank(std::move(audioBank)),
    mSoundSet(std::move(soundSet)),
    mSound(std::move(sound)),
    mSoundID(-1) {
}

GameSound::~GameSound() = default;

void GameSound::Play() {
    if (mSoundID != -1) {
        Stop();
    }

    mSoundID = AUDIO::GET_SOUND_ID();
    AUDIO::PLAY_SOUND_FRONTEND(mSoundID, (char*)mSound.c_str(), (char*)mSoundSet.c_str(), 0);
}

void GameSound::Play(Entity ent) {
    if (mSoundID != -1) {
        Stop();
    }

    mSoundID = AUDIO::GET_SOUND_ID();
    AUDIO::PLAY_SOUND_FROM_ENTITY(mSoundID, (char*)mSound.c_str(), ent, (char*)mSoundSet.c_str(), 0, 0);
}

void GameSound::Stop() {
    if (mSoundID == -1) 
        return;
    AUDIO::STOP_SOUND(mSoundID);
    AUDIO::RELEASE_SOUND_ID(mSoundID);
    mSoundID = -1;
}

bool GameSound::Playing() {
    return mSoundID != -1;
}