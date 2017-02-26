#pragma once

namespace MemoryPatcher {
	bool PatchInstructions();
	bool RestoreInstructions();

	bool PatchSteeringCorrection();
	bool RestoreSteeringCorrection();

	/*
	* "total" refers to the "package" of patches needed to get the gearbox
	* and clutch stuff working.
	*/
	extern int TotalToPatch;
	extern int TotalPatched;

	/*
	 * That means SteeringPatched is just for the steering part.
	 */
	extern bool SteeringPatched;

};
