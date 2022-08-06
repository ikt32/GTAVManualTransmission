#pragma once
#include <inc/types.h>
#include <string>

//https://github.com/CamxxCore/AirSuperiority
class GameSound {
public:
    GameSound(std::string sound, std::string soundSet, std::string audioBank);
    ~GameSound();

    void Play();
    void Play(Entity ent);
    void Stop();
    bool Playing();

private:
    std::string mAudioBank;
    std::string mSoundSet;
    std::string mSound;
    int mSoundID;
};
