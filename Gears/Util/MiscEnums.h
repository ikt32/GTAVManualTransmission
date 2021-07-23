#pragma once

//http://pastebin.com/Kj9t38KF
enum eRadioStations {
    LosSantosRockRadio,
    NonStopPopFM,
    RadioLosSantos,
    ChannelX,
    WestCoastTalkRadio,
    RebelRadio,
    SoulwaxFM,
    EastLosFM,
    WestCoastClassics,
    BlueArk,
    WorldideFM,
    FlyLoFM,
    TheLowdown,
    TheLab,
    RadioMirrorPark,
    Space1032,
    VinewoodBlvdRadio,
    SelfRadio,
    BlaineCountyRadio,
    RadioOff = 255
};

enum eDecorType {
    DECOR_TYPE_FLOAT = 1,
    DECOR_TYPE_BOOL,
    DECOR_TYPE_INT,
    DECOR_TYPE_UNK,
    DECOR_TYPE_TIME
};

enum EVehicleLight : uint32_t {
    LeftHeadlight = 1 << 0,
    RightHeadlight = 1 << 1,

    LeftTailLight = 1 << 2,
    RightTailLight = 1 << 3,

    LeftFrontIndicatorLight = 1 << 4,
    RightFrontIndicatorLight = 1 << 5,
    LeftRearIndicatorLight = 1 << 6,
    RightRearIndicatorLight = 1 << 7,

    LeftBrakeLight = 1 << 8,
    RightBrakeLight = 1 << 9,
    MiddleBrakeLight = 1 << 10,
};
