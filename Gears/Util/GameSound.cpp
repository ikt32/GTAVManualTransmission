#include "GameSound.h"
#include <inc/natives.h>


GameSound::GameSound(char *sound, char *soundSet) {
    Active = false;
    m_sound = sound;
    m_soundSet = soundSet;
    m_soundID = -1;
}

GameSound::~GameSound() {
    if (m_soundID == -1 || !Active) return;
    AUDIO::RELEASE_SOUND_ID(m_soundID);
}

void GameSound::Load(char *audioBank) {
    AUDIO::REQUEST_SCRIPT_AUDIO_BANK(audioBank, false);
}

void GameSound::Play(Entity ent) {
    if (Active) return;
    m_soundID = AUDIO::GET_SOUND_ID();
    AUDIO::PLAY_SOUND_FROM_ENTITY(m_soundID, m_sound, ent, m_soundSet, 0, 0);
    Active = true;
}

void GameSound::Stop() {
    if (m_soundID == -1 || !Active) return;
    AUDIO::STOP_SOUND(m_soundID);
    Active = false;
}
