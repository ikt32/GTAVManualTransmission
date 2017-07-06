#pragma once

#include <string>
#include <vector>
#include "inc/types.h"

struct Color {
	int R;
	int G;
	int B;
	int A;
};

const Color solidWhite = { 255,	255, 255, 255 };
const Color solidBlack = { 0, 0, 0, 255 };

const Color solidRed = { 255, 0, 0,	255 };
const Color solidGreen = { 0, 255, 0, 255 };
const Color solidBlue = { 0, 0, 255, 255 };

const Color solidPink = { 255, 0, 255, 255 };
const Color solidYellow = { 255, 255, 0, 255 };
const Color solidCyan = { 0, 255, 255, 255 };

const Color solidOrange = { 255, 127, 0, 255 };
const Color solidLime = { 127, 255, 0, 255 };
const Color solidPurple = { 127, 0, 255, 255 };

const Color transparentGray = { 75, 75, 75, 75 };


// Natives called
void showText(float x, float y, float scale, std::string text, int font = 0, const Color &rgba = solidWhite, bool outline = false);
void showText(float x, float y, float scale, const char* text, int font = 0, const Color &rgba = solidWhite, bool outline = false);
void showNotification(const char* message, int *prevNotification = nullptr);
void showSubtitle(std::string message, int duration = 2500);

//http://stackoverflow.com/questions/36789380/how-to-store-a-const-char-to-a-char
class CharAdapter
{
public:
	explicit CharAdapter(const char* s) : m_s(::_strdup(s)) { }
	CharAdapter(const CharAdapter& other) = delete; // non construction-copyable
	CharAdapter& operator=(const CharAdapter&) = delete; // non copyable
	
	~CharAdapter() /*free memory on destruction*/
	{
		::free(m_s); /*use free to release strdup memory*/
	}
	operator char*() /*implicit cast to char* */
	{
		return m_s;
	}

private:
	char* m_s;
};

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
	int m_prevNotification;
};

void DisableActionControlsStart();
void DisableActionControlsStop();
void DisableActionControlsTick();

std::string PrettyNameFromHash(Hash hash);

bool FileExists(const std::string& name);
