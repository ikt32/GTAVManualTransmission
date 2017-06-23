#pragma once
#include <string>
#include <vector>

#define DISPLAY_VERSION "v4.3.6"
#define CORRECTVGENERAL "430"
#define CORRECTVWHEEL   "430"

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

	"VER_1_0_1011_1_STEAM", // 30
	"VER_1_0_1011_1_NOSTEAM", // 31

	"VER_1_0_1032_1_STEAM", // 32
	"VER_1_0_1032_1_NOSTEAM", // 33

	"VER_1_0_1103_2_STEAM", // 34
	"VER_1_0_1103_2_NOSTEAM", // 35
};

enum G_GameVersion : int {
	G_VER_1_0_335_2_STEAM, // 00
	G_VER_1_0_335_2_NOSTEAM, // 01

	G_VER_1_0_350_1_STEAM, // 02
	G_VER_1_0_350_2_NOSTEAM, // 03

	G_VER_1_0_372_2_STEAM, // 04
	G_VER_1_0_372_2_NOSTEAM, // 05

	G_VER_1_0_393_2_STEAM, // 06
	G_VER_1_0_393_2_NOSTEAM, // 07

	G_VER_1_0_393_4_STEAM, // 08
	G_VER_1_0_393_4_NOSTEAM, // 09

	G_VER_1_0_463_1_STEAM, // 10
	G_VER_1_0_463_1_NOSTEAM, // 11

	G_VER_1_0_505_2_STEAM, // 12
	G_VER_1_0_505_2_NOSTEAM, // 13

	G_VER_1_0_573_1_STEAM, // 14
	G_VER_1_0_573_1_NOSTEAM, // 15

	G_VER_1_0_617_1_STEAM, // 16
	G_VER_1_0_617_1_NOSTEAM, // 17

	G_VER_1_0_678_1_STEAM, // 18
	G_VER_1_0_678_1_NOSTEAM, // 19

	G_VER_1_0_757_2_STEAM, // 20
	G_VER_1_0_757_2_NOSTEAM, // 21

	G_VER_1_0_757_4_STEAM, // 22
	G_VER_1_0_757_4_NOSTEAM, // 23

	G_VER_1_0_791_2_STEAM, // 24
	G_VER_1_0_791_2_NOSTEAM, // 25

	G_VER_1_0_877_1_STEAM, // 26
	G_VER_1_0_877_1_NOSTEAM, // 27

	G_VER_1_0_944_2_STEAM, // 28
	G_VER_1_0_944_2_NOSTEAM, // 29

	G_VER_1_0_1011_1_STEAM, // 30
	G_VER_1_0_1011_1_NOSTEAM, // 31

	G_VER_1_0_1032_1_STEAM, // 32
	G_VER_1_0_1032_1_NOSTEAM, // 33

	G_VER_1_0_1103_2_STEAM, // 34
	G_VER_1_0_1103_2_NOSTEAM, // 35
};

static std::string eGameVersionToString(int version) {
	if (version > GameVersionString.size() - 1 || version < 0) {
		return std::to_string(version);
	}
	return GameVersionString[version];
}
