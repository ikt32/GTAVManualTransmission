#pragma once

#ifdef MT_EXPORTS
#define MT_API extern "C" __declspec(dllexport)
#else
#define MT_API extern "C" __declspec(dllimport)
#endif

/**
 * \brief           Get the mod version as a string.
 * \return          A null-terminated char array.
 */
MT_API const char*  MT_GetVersion();

/**
 * \brief           Get whether the Manual Transmission part is active or not.
 *                  When active, shifting behavior on all vehicles is patched,
 *                  and is controlled by the mod.
 * \return          True: MT is active. False: MT is not active.
 */
MT_API bool         MT_IsActive();

/**
 * \brief           Turn on/off the Manual Transmission part of this mod.
 * \param [in] active Whether Manual Transmission should be ON or OFF.
 */
MT_API void         MT_SetActive(bool active);

/**
 * \brief           Get if the gear is in neutral. Neutral is specific to this
 *                  mod, so it can't be read from the game itself.
 * \return          True if in neutral gear.
 */
MT_API bool         MT_NeutralGear();

/**
 * \brief           Get current shifting mode.
 *                  1: Sequential
 *                  2: H-Pattern
 *                  3: Automatic
 * \return          Shifting mode.
 */
MT_API int          MT_GetShiftMode();

/**
 * \brief           Set the shifting mode.
 *                  1: Sequential
 *                  2: H-Pattern
 *                  3: Automatic
 * \param [in] mode Desired mode.
 */
MT_API void         MT_SetShiftMode(int mode);

/**
 * \brief           Get whether the user should shift up/down
 *                  0: No action
 *                  1: Shift up
 *                  2: Shift down
 */
MT_API int          MT_GetShiftIndicator();

/**
 * \brief           Get the eco rate for the auto gearbox.
 * \return          The eco rate value, likely between 0.01 and 0.50
 */
MT_API float        MT_GetAutoEcoRate();

/**
 * \brief           Set the eco rate for the auto gearbox.
 *                  Warning: Settings will save after this call!
 * \param [in] rate The desired rate. Recommended between 0.01 and 0.50
 */
MT_API void         MT_SetAutoEcoRate(float rate);

/*
 * AI shifting override
 */

/**
 * \brief           Add a specific vehicle that the AI shifting code should
 *                  ignore. Use this for when you want your own shifting
 *                  logic to apply to the vehicle instance.
 * \param [in] vehicle Vehicle to ignore.
 */
MT_API void         MT_AddIgnoreVehicle(int vehicle);

/**
 * \brief           Remove a specific vehicle from AI shift control ignore.
 * \param [in] vehicle Vehicle to ignore
 */
MT_API void         MT_DelIgnoreVehicle(int vehicle);

/**
 * \brief           Return all vehicles back to AI shifting control.
 */
MT_API void         MT_ClearIgnoredVehicles();

/**
 * \brief           Get the number of vehicles that AI shifting ignores.
 * \return          Size of MT_GetIgnoredVehicles.
 */
MT_API unsigned     MT_NumIgnoredVehicles();

/**
 * \brief           Get the vehicles that AI shifting ignores.
 * \return          Array of ignored Vehicles.
 */
MT_API const int*   MT_GetIgnoredVehicles();

/**
 * \brief           Gets the vehicle under control of the script.
 * \return          Handle to the Vehicle entity the player is using.
 */
MT_API int          MT_GetManagedVehicle();

/*
 * Basic steering wheel input info.
 */

/**
 * \brief           Gets whether "look left" is pressed on the steering wheel
 * \return          True for pressed, false for not.
 */
MT_API bool MT_LookingLeft();

/**
 * \brief           Gets whether "look right" is pressed on the steering wheel
 * \return          True for pressed, false for not.
 */
MT_API bool MT_LookingRight();

/**
 * \brief           Gets whether "look back" is pressed on the steering wheel
 * \return          True for pressed, false for not.
 */
MT_API bool MT_LookingBack();
