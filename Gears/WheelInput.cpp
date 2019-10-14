#include "WheelInput.h"

#include "script.h"
#include "ScriptUtils.h"
#include "ScriptSettings.hpp"
#include "VehicleData.hpp"
#include "Input/CarControls.hpp"
#include "Util/MathExt.h"
#include "Util/MiscEnums.h"
#include "Util/UIUtils.h"
#include "Memory/VehicleExtensions.hpp"
#include "Memory/Offsets.hpp"

#include "inc/enums.h"
#include "inc/natives.h"
#include "inc/types.h"

#include "MiniPID/MiniPID.h"
#include "fmt/format.h"
#include <algorithm>

extern Vehicle playerVehicle;
extern Ped playerPed;
extern VehicleExtensions ext;
extern ScriptSettings settings;
extern VehicleData vehData;
extern CarControls carControls;

extern VehiclePeripherals peripherals;
extern VehicleGearboxStates gearStates;
extern WheelPatchStates wheelPatchStates;

namespace {
    MiniPID pid(1.0, 0.0, 0.0);
}

namespace WheelInput {

}

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Reverse/Pedal handling
///////////////////////////////////////////////////////////////////////////////

// Forward gear: Throttle accelerates, Brake brakes (exclusive)
// Reverse gear: Throttle reverses, Brake brakes (exclusive)
void WheelInput::HandlePedals(float wheelThrottleVal, float wheelBrakeVal) {
    wheelThrottleVal = pow(wheelThrottleVal, settings.ThrottleGamma);
    wheelBrakeVal = pow(wheelBrakeVal, settings.BrakeGamma);

    float speedThreshold = 0.5f;
    const float reverseThreshold = 2.0f;

    if (vehData.mGearCurr > 0) {
        // Going forward
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y > speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are going forward");
            // Throttle Pedal normal
            if (wheelThrottleVal > 0.01f) {
                Controls::SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, carControls.ADZThrottle);
            }
            // Brake Pedal normal
            if (wheelBrakeVal > 0.01f) {
                Controls::SetControlADZ(ControlVehicleBrake, wheelBrakeVal, carControls.ADZBrake);
            }
        }

        // Standing still
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y < speedThreshold && ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y >= -speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are stopped");

            if (wheelThrottleVal > 0.01f) {
                Controls::SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, carControls.ADZThrottle);
            }

            if (wheelBrakeVal > 0.01f) {
                ext.SetThrottleP(playerVehicle, 0.1f);
                ext.SetBrakeP(playerVehicle, 1.0f);
                VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(playerVehicle, true);
            }
        }

        // Rolling back
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y < -speedThreshold) {
            bool brakelights = false;
            // Just brake
            if (wheelThrottleVal <= 0.01f && wheelBrakeVal > 0.01f) {
                //showText(0.3, 0.0, 1.0, "We should brake");
                //showText(0.3, 0.05, 1.0, ("Brake pressure:" + std::to_string(wheelBrakeVal)));
                Controls::SetControlADZ(ControlVehicleAccelerate, wheelBrakeVal, carControls.ADZBrake);
                ext.SetThrottleP(playerVehicle, 0.1f);
                ext.SetBrakeP(playerVehicle, 1.0f);
                brakelights = true;
            }

            if (!gearStates.FakeNeutral && wheelThrottleVal > 0.01f && carControls.ClutchVal < settings.ClutchThreshold) {
                //showText(0.3, 0.0, 1.0, "We should burnout");
                if (carControls.BrakeVal < 0.1f) {
                    VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(playerVehicle, false);
                }
                for (int i = 0; i < vehData.mWheelCount; i++) {
                    if (ext.IsWheelPowered(playerVehicle, i)) {
                        ext.SetWheelBrakePressure(playerVehicle, i, 0.0f);
                        ext.SetWheelPower(playerVehicle, i, 2.0f * ext.GetDriveForce(playerVehicle));
                    }
                    else {
                        float handlingBrakeForce = *reinterpret_cast<float*>(vehData.mHandlingPtr + hOffsets.fBrakeForce);
                        float inpBrakeForce = handlingBrakeForce * carControls.BrakeVal;
                        ext.SetWheelPower(playerVehicle, i, 0.0f);
                        ext.SetWheelBrakePressure(playerVehicle, i, inpBrakeForce);
                    }
                }
                fakeRev();
                ext.SetThrottle(playerVehicle, carControls.ThrottleVal);
                ext.SetThrottleP(playerVehicle, carControls.ThrottleVal);
                wheelPatchStates.InduceBurnout = true;
            }

            if (wheelThrottleVal > 0.01f && (carControls.ClutchVal > settings.ClutchThreshold || gearStates.FakeNeutral)) {
                if (wheelBrakeVal > 0.01f) {
                    //showText(0.3, 0.0, 1.0, "We should rev and brake");
                    //showText(0.3, 0.05, 1.0, ("Brake pressure:" + std::to_string(wheelBrakeVal)) );
                    Controls::SetControlADZ(ControlVehicleAccelerate, wheelBrakeVal, carControls.ADZBrake);
                    ext.SetThrottleP(playerVehicle, 0.1f);
                    ext.SetBrakeP(playerVehicle, 1.0f);
                    brakelights = true;
                    fakeRev(false, 0);
                }
                else if (carControls.ClutchVal > 0.9 || gearStates.FakeNeutral) {
                    //showText(0.3, 0.0, 1.0, "We should rev and do nothing");
                    ext.SetThrottleP(playerVehicle, wheelThrottleVal);
                    fakeRev(false, 0);
                }
                else {
                    //showText(0.3, 0.0, 1.0, "We should rev and apply throttle");
                    Controls::SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, carControls.ADZThrottle);
                    ext.SetThrottleP(playerVehicle, wheelThrottleVal);
                    fakeRev(false, 0);
                }
            }
            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(playerVehicle, brakelights);
        }
        else {
            wheelPatchStates.InduceBurnout = false;
        }
    }

    if (vehData.mGearCurr == 0) {
        // Enables reverse lights
        ext.SetThrottleP(playerVehicle, -0.1f);

        // We're reversing
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y < -speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are reversing");
            // Throttle Pedal Reverse
            if (wheelThrottleVal > 0.01f) {
                Controls::SetControlADZ(ControlVehicleBrake, wheelThrottleVal, carControls.ADZThrottle);
            }
            // Brake Pedal Reverse
            if (wheelBrakeVal > 0.01f) {
                Controls::SetControlADZ(ControlVehicleAccelerate, wheelBrakeVal, carControls.ADZBrake);
                ext.SetThrottleP(playerVehicle, -wheelBrakeVal);
                ext.SetBrakeP(playerVehicle, 1.0f);
            }
        }

        // Standing still
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y < speedThreshold && ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y >= -speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are stopped");

            if (wheelThrottleVal > 0.01f) {
                Controls::SetControlADZ(ControlVehicleBrake, wheelThrottleVal, carControls.ADZThrottle);
            }

            if (wheelBrakeVal > 0.01f) {
                ext.SetThrottleP(playerVehicle, -wheelBrakeVal);
                ext.SetBrakeP(playerVehicle, 1.0f);
                VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(playerVehicle, true);
            }
        }

        // We're rolling forwards
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y > speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are rolling forwards");
            //bool brakelights = false;

            if (ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y > reverseThreshold) {
                if (carControls.ClutchVal < settings.ClutchThreshold) {
                    CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, 1.0f);
                }
                // Brake Pedal Reverse
                if (wheelBrakeVal > 0.01f) {
                    Controls::SetControlADZ(ControlVehicleBrake, wheelBrakeVal, carControls.ADZBrake);
                    ext.SetThrottleP(playerVehicle, -wheelBrakeVal);
                    ext.SetBrakeP(playerVehicle, 1.0f);
                }
            }

            //VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, brakelights);
        }
    }
}

// Pedals behave like RT/LT
void WheelInput::HandlePedalsArcade(float wheelThrottleVal, float wheelBrakeVal) {
    wheelThrottleVal = pow(wheelThrottleVal, settings.ThrottleGamma);
    wheelBrakeVal = pow(wheelBrakeVal, settings.BrakeGamma);
    if (wheelThrottleVal > 0.01f) {
        Controls::SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, carControls.ADZThrottle);
    }
    if (wheelBrakeVal > 0.01f) {
        Controls::SetControlADZ(ControlVehicleBrake, wheelBrakeVal, carControls.ADZBrake);
    }
}


///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Buttons
///////////////////////////////////////////////////////////////////////////////

// Set state, and activate them if hazards are off.
void setBlinkers(bool left, bool right) {
    peripherals.BlinkerLeft = left;
    peripherals.BlinkerRight = right;

    if (!peripherals.BlinkerHazard) {
        VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(playerVehicle, 0, right);
        VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(playerVehicle, 1, left);
    }
}

void checkIndicatorActions() {
    if (carControls.ButtonJustPressed(CarControls::WheelControlType::IndicatorHazard)) {
        if (!peripherals.BlinkerHazard) {
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(playerVehicle, 0, true);
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(playerVehicle, 1, true);
            peripherals.BlinkerHazard = true;
        }
        else {
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(playerVehicle, 0, peripherals.BlinkerRight);
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(playerVehicle, 1, peripherals.BlinkerLeft);
            peripherals.BlinkerHazard = false;
        }
    }

    if (carControls.ButtonJustPressed(CarControls::WheelControlType::IndicatorLeft)) {
        if (!peripherals.BlinkerLeft) {
            peripherals.BlinkerTicks = 1;
            setBlinkers(true, false);
        }
        else {
            peripherals.BlinkerTicks = 0;
            setBlinkers(false, false);
        }
    }
    if (carControls.ButtonJustPressed(CarControls::WheelControlType::IndicatorRight)) {
        if (!peripherals.BlinkerRight) {
            peripherals.BlinkerTicks = 1;
            setBlinkers(false, true);
        }
        else {
            peripherals.BlinkerTicks = 0;
            setBlinkers(false, false);
        }
    }

    float wheelCenterDeviation = (carControls.SteerVal - 0.5f) / 0.5f;

    if (peripherals.BlinkerTicks == 1 && abs(wheelCenterDeviation) > 0.2f) {
        peripherals.BlinkerTicks = 2;
    }

    if (peripherals.BlinkerTicks == 2 && abs(wheelCenterDeviation) < 0.1f) {
        peripherals.BlinkerTicks = 0;
        setBlinkers(false, false);
    }
}

void checkRadioButtons() {
    if (carControls.ButtonHeld(CarControls::WheelControlType::RadioPrev, 1000) ||
        carControls.ButtonHeld(CarControls::WheelControlType::RadioNext, 1000)) {
        if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() != RadioOff) {
            peripherals.RadioStationIndex = AUDIO::GET_PLAYER_RADIO_STATION_INDEX();
        }
        AUDIO::SET_VEH_RADIO_STATION(playerVehicle, "OFF");
        return;
    }
    if (carControls.ButtonReleased(CarControls::WheelControlType::RadioNext)) {
        if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() == RadioOff) {
            AUDIO::SET_RADIO_TO_STATION_INDEX(peripherals.RadioStationIndex);
            return;
        }
        AUDIO::_0xFF266D1D0EB1195D(); // Next radio station
    }
    if (carControls.ButtonReleased(CarControls::WheelControlType::RadioPrev)) {
        if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() == RadioOff) {
            AUDIO::SET_RADIO_TO_STATION_INDEX(peripherals.RadioStationIndex);
            return;
        }
        AUDIO::_0xDD6BCF9E94425DF9(); // Prev radio station
    }
}

void checkCameraButtons() {
    if (carControls.ButtonIn(CarControls::WheelControlType::LookBack)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleLookBehind, 1.0f);
    }

    // who was first?
    if (carControls.ButtonIn(CarControls::WheelControlType::LookRight) &&
        carControls.ButtonJustPressed(CarControls::WheelControlType::LookLeft)) {
        peripherals.LookBackRShoulder = true;
    }

    if (carControls.ButtonIn(CarControls::WheelControlType::LookLeft) &&
        carControls.ButtonIn(CarControls::WheelControlType::LookRight)) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(peripherals.LookBackRShoulder ? -180.0f : 180.0f);
    }
    else if (carControls.ButtonIn(CarControls::WheelControlType::LookLeft)) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_PITCH(0.0f, 1.0f);
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(90.0f);
    }
    else if (carControls.ButtonIn(CarControls::WheelControlType::LookRight)) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_PITCH(0.0f, 1.0f);
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(-90.0f);
    }
    if (carControls.ButtonReleased(CarControls::WheelControlType::LookLeft) && !carControls.ButtonIn(CarControls::WheelControlType::LookRight) ||
        carControls.ButtonReleased(CarControls::WheelControlType::LookRight) && !carControls.ButtonIn(CarControls::WheelControlType::LookLeft)) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(0.0f);
    }
    if (carControls.ButtonReleased(CarControls::WheelControlType::LookLeft) ||
        carControls.ButtonReleased(CarControls::WheelControlType::LookRight)) {
        peripherals.LookBackRShoulder = false;
    }

    if (carControls.ButtonJustPressed(CarControls::WheelControlType::Camera)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlNextCamera, 1.0f);
    }
}

void checkVehicleInputButtons() {
    if (carControls.HandbrakeVal > 0.1f) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, carControls.HandbrakeVal);
    }
    if (carControls.ButtonIn(CarControls::WheelControlType::Handbrake)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, 1.0f);
    }
    if (carControls.ButtonIn(CarControls::WheelControlType::Horn)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHorn, 1.0f);
    }
    if (carControls.ButtonJustPressed(CarControls::WheelControlType::Lights)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHeadlight, 1.0f);
    }
}

void WheelInput::CheckButtons() {
    if (carControls.PrevInput != CarControls::Wheel) {
        return;
    }

    checkVehicleInputButtons();
    checkCameraButtons();
    checkRadioButtons();
    checkIndicatorActions();
}

///////////////////////////////////////////////////////////////////////////////
//                    Wheel input and force feedback
///////////////////////////////////////////////////////////////////////////////

void WheelInput::DoSteering() {
    if (carControls.PrevInput != CarControls::Wheel)
        return;

    float steerMult;
    if (vehData.mClass == VehicleClass::Bike || vehData.mClass == VehicleClass::Quad)
        steerMult = settings.SteerAngleMax / settings.SteerAngleBike;
    else if (vehData.mClass == VehicleClass::Car)
        steerMult = settings.SteerAngleMax / settings.SteerAngleCar;
    else {
        steerMult = settings.SteerAngleMax / settings.SteerAngleBoat;
    }

    float steerValL = map(carControls.SteerVal, 0.0f, 0.5f, 1.0f, 0.0f);
    float steerValR = map(carControls.SteerVal, 0.5f, 1.0f, 0.0f, 1.0f);
    float steerValGammaL = pow(steerValL, settings.SteerGamma);
    float steerValGammaR = pow(steerValR, settings.SteerGamma);
    float steerValGamma = carControls.SteerVal < 0.5f ? -steerValGammaL : steerValGammaR;
    float effSteer = steerMult * steerValGamma;

    /*
     * Patched steering is direct without any processing, and super direct.
     * _SET_CONTROL_NORMAL is with game processing and could have a bit of assist
     * Both should work without any deadzone, with a note that the second one
     * does need a specified anti-deadzone (recommended: 24-25%)
     */
    if (vehData.mClass == VehicleClass::Car) {
        ext.SetSteeringAngle(playerVehicle, -effSteer * ext.GetMaxSteeringAngle(playerVehicle));
        ext.SetSteeringInputAngle(playerVehicle, -effSteer);
    }
    else {
        Controls::SetControlADZ(ControlVehicleMoveLeftRight, effSteer, carControls.ADZSteer);
    }
}

int calculateDamper(float gain, float wheelsOffGroundRatio) {
    Vector3 accelValsAvg = vehData.mAcceleration; //TODO: Avg/dampen later

    // Just to use floats everywhere here
    float damperMax = static_cast<float>(settings.DamperMax);
    float damperMin = static_cast<float>(settings.DamperMin);
    float damperMinSpeed = static_cast<float>(settings.DamperMinSpeed);

    float absVehicleSpeed = abs(ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true).y);
    float damperFactorSpeed = map(absVehicleSpeed, 0.0f, damperMinSpeed, damperMax, damperMin);
    damperFactorSpeed = fmaxf(damperFactorSpeed, damperMin);
    // already clamped on the upper bound by abs(vel) in map()

    // 2G of deceleration causes addition of damperMin. Heavier feeling steer when braking!
    float damperFactorAccel = map(-accelValsAvg.y / 9.81f, -2.0f, 2.0f, -damperMin, damperMin);
    damperFactorAccel = fmaxf(damperFactorAccel, -damperMin);
    damperFactorAccel = fminf(damperFactorAccel, damperMin);

    float damperForce = damperFactorSpeed + damperFactorAccel;

    damperForce = damperForce * (1.0f - wheelsOffGroundRatio);

    if (vehData.mClass == VehicleClass::Car || vehData.mClass == VehicleClass::Quad) {
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(playerVehicle, 0, true)) {
            damperForce /= 2.0f;
        }
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(playerVehicle, 1, true)) {
            damperForce /= 2.0f;
        }
    }
    else if (vehData.mClass == VehicleClass::Bike) {
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(playerVehicle, 0, true)) {
            damperForce /= 4.0f;
        }
    }

    if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(playerVehicle)) {
        damperForce *= 2.0f;
    }

    // clamp
    damperForce = fmaxf(damperForce, damperMin / 4.0f);
    damperForce = fminf(damperForce, damperMax * 2.0f);

    damperForce *= gain;

    return static_cast<int>(damperForce);
}

int calculateDetail() {
    // Detail feel / suspension compression based
    float compSpeedTotal = 0.0f;
    auto compSpeed = vehData.mSuspensionTravelSpeeds;

    // More than 2 wheels! Trikes should be ok, etc.
    if (compSpeed.size() > 2) {
        // left should pull left, right should pull right
        compSpeedTotal = -compSpeed[0] + compSpeed[1];
    }

    return static_cast<int>(1000.0f * settings.DetailMult * compSpeedTotal);
}

void calculateSoftLock(int& totalForce, int& damperForce) {
    float steerMult;
    if (vehData.mClass == VehicleClass::Bike || vehData.mClass == VehicleClass::Quad)
        steerMult = settings.SteerAngleMax / settings.SteerAngleBike;
    else if (vehData.mClass == VehicleClass::Car)
        steerMult = settings.SteerAngleMax / settings.SteerAngleCar;
    else {
        steerMult = settings.SteerAngleMax / settings.SteerAngleBoat;
    }
    float effSteer = steerMult * 2.0f * (carControls.SteerVal - 0.5f);
    float steerSpeed = carControls.GetAxisSpeed(CarControls::WheelAxisType::Steer);
    if (effSteer > 1.0f) {
        if (steerSpeed > 0.1f) {
            damperForce = (int)map(steerSpeed, 0.1f, 0.4f, (float)damperForce, 40000.0f);
        }
        totalForce = (int)map(effSteer, 1.0f, steerMult, (float)totalForce, 40000.0f);
    }
    else if (effSteer < -1.0f) {
        if (steerSpeed < -0.1f) {
            damperForce = (int)map(steerSpeed, -0.1f, -0.4f, (float)damperForce, 40000.0f);
        }
        totalForce = (int)map(effSteer, -1.0f, -steerMult, (float)totalForce, -40000.0f);
    }
}

// Despite being scientifically inaccurate, "self-aligning torque" is the best description.
int calculateSat(int defaultGain, float steeringAngle, float wheelsOffGroundRatio, bool isCar) {
    float speed = ENTITY::GET_ENTITY_SPEED(playerVehicle);
    pid.setD(static_cast<double>(speed) * 0.1);
    Vector3 speedVector = ENTITY::GET_ENTITY_SPEED_VECTOR(playerVehicle, true);
    Vector3 rotVector = ENTITY::GET_ENTITY_ROTATION_VELOCITY(playerVehicle);
    Vector3 rotRelative{
        speed * -sin(rotVector.z), 0,
        speed * cos(rotVector.z), 0,
        0, 0
    };

    Vector3 expectedVector{
        speed * -sin(steeringAngle / settings.GameSteerMultWheel), 0,
        speed * cos(steeringAngle / settings.GameSteerMultWheel), 0,
        0, 0
    };

    float error = static_cast<float>(pid.getOutput(expectedVector.x, static_cast<double>(speedVector.x) * 0.5));

    int satForce = static_cast<int>(settings.SATAmpMult * static_cast<float>(defaultGain) * -error);

    // "Reduction" effects - those that affect already calculated things
    bool understeering = false;
    float understeer = 0.0f;
    if (isCar) {
        float expTurnX = speedVector.x * 0.5f + rotRelative.x * 0.5f;
        understeer = sgn(speedVector.x - expectedVector.x) * (expTurnX - expectedVector.x);
        if (expectedVector.x > expTurnX && expTurnX > speedVector.x ||
            expectedVector.x < expTurnX && expTurnX < speedVector.x) {
            satForce = static_cast<int>(static_cast<float>(satForce) / std::max(1.0f, 3.3f * understeer + 1.0f));
            understeering = true;
        }
    }

    satForce = static_cast<int>(static_cast<float>(satForce) * (1.0f - wheelsOffGroundRatio));

    if (vehData.mClass == VehicleClass::Car || vehData.mClass == VehicleClass::Quad) {
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(playerVehicle, 0, true)) {
            satForce = satForce / 4;
        }
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(playerVehicle, 1, true)) {
            satForce = satForce / 4;
        }
    }
    else if (vehData.mClass == VehicleClass::Bike) {
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(playerVehicle, 0, true)) {
            satForce = satForce / 10;
        }
    }

    if (settings.DisplayInfo) {
        //showText(0.85, 0.175, 0.4, fmt::format("RelSteer:\t{:.3f}", steeringRelative.x), 4);
        //showText(0.85, 0.200, 0.4, fmt::format("SetPoint:\t{:.3f}", travelRelative.x), 4);
        showText(0.85, 0.225, 0.4, fmt::format("Error:\t\t{:.3f}", error), 4);
        showText(0.85, 0.250, 0.4, fmt::format("{}Under:\t\t{:.3f}~w~", understeering ? "~b~" : "~w~", understeer), 4);
    }
    if (satForce > 0) {
        satForce = map(satForce, 0, 10000, settings.FFB.AntiDeadForce, 10000);
    }
    if (satForce < 0) {
        satForce = map(satForce, -10000, 0, -10000, -settings.FFB.AntiDeadForce);
    }
    satForce = std::clamp(satForce, -settings.FFB.SATMax, settings.FFB.SATMax);

    return satForce;
}

// Despite being scientifically inaccurate, "self-aligning torque" is the best description.
void WheelInput::DrawDebugLines() {
    float steeringAngle = ext.GetWheelAverageAngle(playerVehicle);
    Vector3 velocityWorld = ENTITY::GET_ENTITY_VELOCITY(playerVehicle);
    Vector3 positionWorld = ENTITY::GET_ENTITY_COORDS(playerVehicle, 1);
    Vector3 travelWorld = velocityWorld + positionWorld;
    Vector3 travelRelative = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(playerVehicle, travelWorld.x, travelWorld.y, travelWorld.z);

    Vector3 rotationVelocity = ENTITY::GET_ENTITY_ROTATION_VELOCITY(playerVehicle);
    Vector3 turnWorld = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(playerVehicle, ENTITY::GET_ENTITY_SPEED(playerVehicle) * -sin(rotationVelocity.z), ENTITY::GET_ENTITY_SPEED(playerVehicle) * cos(rotationVelocity.z), 0.0f);
    Vector3 turnRelative = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(playerVehicle, turnWorld.x, turnWorld.y, turnWorld.z);
    float turnRelativeNormX = (travelRelative.x + turnRelative.x) / 2.0f;
    float turnRelativeNormY = (travelRelative.y + turnRelative.y) / 2.0f;
    Vector3 turnWorldNorm = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(playerVehicle, turnRelativeNormX, turnRelativeNormY, 0.0f);

    float steeringAngleRelX = ENTITY::GET_ENTITY_SPEED(playerVehicle) * -sin(steeringAngle);
    float steeringAngleRelY = ENTITY::GET_ENTITY_SPEED(playerVehicle) * cos(steeringAngle);
    Vector3 steeringWorld = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(playerVehicle, steeringAngleRelX, steeringAngleRelY, 0.0f);

    GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, travelWorld.x, travelWorld.y, travelWorld.z, 0, 255, 0, 255);
    GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, turnWorldNorm.x, turnWorldNorm.y, turnWorldNorm.z, 255, 0, 0, 255);
    GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, steeringWorld.x, steeringWorld.y, steeringWorld.z, 255, 0, 255, 255);
}

float getFloatingSteeredWheelsRatio(Vehicle v) {
    auto suspensionStates = vehData.mWheelsOnGround;
    auto angles = vehData.mWheelSteeringAngles;

    float wheelsOffGroundRatio = 0.0f;
    float wheelsInAir = 0.0f;
    float wheelsSteered = 0.0f;

    for (int i = 0; i < vehData.mWheelCount; i++) {
        if (angles[i] != 0.0f) {
            wheelsSteered += 1.0f;
            if (suspensionStates[i] == false) {
                wheelsInAir += 1.0f;
            }
        }
    }
    if (wheelsSteered != 0.0f) {
        wheelsOffGroundRatio = wheelsInAir / wheelsSteered;
    }
    return wheelsOffGroundRatio;
}

float getSteeringLock() {
    switch (vehData.mClass) {
    case VehicleClass::Bike:
    case VehicleClass::Quad:
    case VehicleClass::Bicycle:
        return settings.SteerAngleBike;
    case VehicleClass::Car:
        return settings.SteerAngleCar;
    case VehicleClass::Boat:
        return settings.SteerAngleBoat;
    case VehicleClass::Plane:
    case VehicleClass::Heli:
    case VehicleClass::Train:
    case VehicleClass::Unknown:
    default: return settings.SteerAngleCar;
    }
}

void WheelInput::PlayFFBGround() {
    if (!settings.EnableFFB ||
        carControls.PrevInput != CarControls::Wheel) {
        return;
    }

    float rotationScale = 1.0f;
    if (settings.ScaleFFB) {
        rotationScale = getSteeringLock() / settings.SteerAngleMax * 0.66f + 0.34f;
    }

    if (settings.LogiLEDs) {
        carControls.PlayLEDs(vehData.mRPM, 0.45f, 0.95f);
    }

    float avgAngle = ext.GetWheelAverageAngle(playerVehicle);
    float wheelsOffGroundRatio = getFloatingSteeredWheelsRatio(playerVehicle);

    int detailForce = std::clamp(calculateDetail(), -settings.FFB.DetailLim, settings.FFB.DetailLim);
    int satForce = calculateSat(2500, avgAngle, wheelsOffGroundRatio, vehData.mClass == VehicleClass::Car);
    int damperForce = calculateDamper(50.0f, wheelsOffGroundRatio);

    // Decrease damper if sat rises, so constantForce doesn't fight against damper
    float damperMult = 1.0f - std::min(fabs((float)satForce), 10000.0f) / 10000.0f;
    damperForce = (int)(damperMult * (float)damperForce);

    int totalForce = satForce + detailForce;
    totalForce = (int)((float)totalForce * rotationScale);
    calculateSoftLock(totalForce, damperForce);
    carControls.PlayFFBDynamics(std::clamp(totalForce, -10000, 10000), std::clamp(damperForce, -10000, 10000));

    const float minGforce = 5.0f;
    const float maxGforce = 50.0f;
    const float minForce = 500.0f;
    const float maxForce = 10000.0f;
    float gForce = abs(vehData.mAcceleration.y) / 9.81f; // TODO: Average/dampen later
    bool collision = gForce > minGforce;
    int res = static_cast<int>(map(gForce, minGforce, maxGforce, minForce, maxForce) * settings.CollisionMult);
    if (collision) {
        carControls.PlayFFBCollision(std::clamp(res, -10000, 10000));
    }

    if (settings.DisplayInfo && collision) {
        UI::Notify(fmt::format("Collision @ ~r~{:.3f}G~w~~n~"
            "FFB: {}", gForce, res));
    }
    if (settings.DisplayInfo) {
        showText(0.85, 0.275, 0.4, fmt::format("{}FFBSat:\t\t{}~w~", abs(satForce) > settings.FFB.SATMax ? "~r~" : "~w~", satForce), 4);
        showText(0.85, 0.300, 0.4, fmt::format("{}FFBFin:\t\t{}~w~", abs(totalForce) > 10000 ? "~r~" : "~w~", totalForce), 4);
        showText(0.85, 0.325, 0.4, fmt::format("Damper:\t\t{}", damperForce), 4);
        showText(0.85, 0.350, 0.4, fmt::format("Detail:\t\t{}", detailForce), 4);
    }
}

void WheelInput::PlayFFBWater() {
    if (!settings.EnableFFB ||
        carControls.PrevInput != CarControls::Wheel) {
        return;
    }

    float rotationScale = 1.0f;
    if (settings.ScaleFFB) {
        rotationScale = getSteeringLock() / settings.SteerAngleMax * 0.66f + 0.34f;
    }

    if (settings.LogiLEDs) {
        carControls.PlayLEDs(vehData.mRPM, 0.45f, 0.95f);
    }

    auto suspensionStates = vehData.mWheelsOnGround;
    auto angles = vehData.mWheelSteeringAngles;

    bool isInWater = ENTITY::GET_ENTITY_SUBMERGED_LEVEL(playerVehicle) > 0.10f;
    int damperForce = calculateDamper(50.0f, isInWater ? 0.25f : 1.0f);
    int detailForce = calculateDetail();
    int satForce = calculateSat(750, ENTITY::GET_ENTITY_ROTATION_VELOCITY(playerVehicle).z, 1.0f, false);

    if (!isInWater) {
        satForce = 0;
    }

    int totalForce = satForce + detailForce;
    totalForce = (int)((float)totalForce * rotationScale);
    calculateSoftLock(totalForce, damperForce);
    carControls.PlayFFBDynamics(totalForce, damperForce);

    if (settings.DisplayInfo) {
        showText(0.85, 0.275, 0.4, fmt::format("{}FFBSat:\t\t{}~w~", abs(satForce) > 10000 ? "~r~" : "~w~", satForce), 4);
        showText(0.85, 0.300, 0.4, fmt::format("{}FFBFin:\t\t{}~w~", abs(totalForce) > 10000 ? "~r~" : "~w~", totalForce), 4);
        showText(0.85, 0.325, 0.4, fmt::format("Damper:\t{}", damperForce), 4);
    }
}
