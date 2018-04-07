#pragma once
#include <inc/types.h>

//https://github.com/CamxxCore/AirSuperiority
class GameSound {
public:
    GameSound(char *sound, char *soundSet);
    ~GameSound();
    void Load(char *audioBank);
    void Play(Entity ent);
    void Stop();

    bool Active;

private:
    char *m_soundSet;
    char *m_sound;
    int m_soundID;
};
