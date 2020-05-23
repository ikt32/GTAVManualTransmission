struct AtcuGearbox {
    float parsePowerIntersectionRpm(int gear);
    float rpmPredictSpeed(int gear, float rpm);

    float upshiftingIndex = 0.0f;
    float downshiftingIndex = 0.0f;
};
