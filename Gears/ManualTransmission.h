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
