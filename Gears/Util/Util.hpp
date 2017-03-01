#pragma once

#include <string>
#include <vector>

static std::vector<std::string> GameVersionString = {
	"VER_1_0_335_2_STEAM", // 00
	"VER_1_0_335_2_NOSTEAM", // 01

	"VER_1_0_350_1_STEAM", // 02
	"VER_1_0_350_2_NOSTEAM", // 03

	"VER_1_0_372_2_STEAM", // 04
	"VER_1_0_372_2_NOSTEAM", // 05

	"VER_1_0_393_2_STEAM", // 06
	"VER_1_0_393_2_NOSTEAM", // 07

	"VER_1_0_393_4_STEAM", // 08
	"VER_1_0_393_4_NOSTEAM", // 09

	"VER_1_0_463_1_STEAM", // 10
	"VER_1_0_463_1_NOSTEAM", // 11

	"VER_1_0_505_2_STEAM", // 12
	"VER_1_0_505_2_NOSTEAM", // 13
	
	"VER_1_0_573_1_STEAM", // 14
	"VER_1_0_573_1_NOSTEAM", // 15

	"VER_1_0_617_1_STEAM", // 16
	"VER_1_0_617_1_NOSTEAM", // 17

	"VER_1_0_678_1_STEAM", // 18
	"VER_1_0_678_1_NOSTEAM", // 19

	"VER_1_0_757_2_STEAM", // 20
	"VER_1_0_757_2_NOSTEAM", // 21

	"VER_1_0_757_4_STEAM", // 22
	"VER_1_0_757_4_NOSTEAM", // 23

	"VER_1_0_791_2_STEAM", // 24
	"VER_1_0_791_2_NOSTEAM", // 25

	"VER_1_0_877_1_STEAM", // 26
	"VER_1_0_877_1_NOSTEAM", // 27

	"VER_1_0_944_2_STEAM", // 28
	"VER_1_0_944_2_NOSTEAM", // 29

	"VER_1_0_975_1_STEAM", // 30
	"VER_1_0_975_1_NOSTEAM" // 31
};

static std::string eGameVersionToString(int version) {
	if (version > GameVersionString.size() - 1) {
		return std::to_string(version);
	}
	return GameVersionString[version];
}

struct Color {
	int R;
	int G;
	int B;
	int A;
};