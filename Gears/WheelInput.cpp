#include "WheelInput.h"

#include "VehicleConfig.h"
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
#include "Memory/VehicleBone.h"
#include "SteeringAnim.h"

extern Vehicle g_playerVehicle;
extern Ped g_playerPed;
extern VehicleExtensions g_ext;
extern ScriptSettings g_settings;
extern VehicleConfig* g_activeConfig;
extern VehicleData g_vehData;
extern CarControls g_controls;

extern VehiclePeripherals g_peripherals;
extern VehicleGearboxStates g_gearStates;
extern WheelPatchStates g_wheelPatchStates;

namespace {
    MiniPID pid(1.0, 0.0, 0.0);
}

namespace WheelInput {
    // Use alternative throttle - brake - 
    void SetControlADZAlt(eControl control, float value, float adz, bool alt) {
        Controls::SetControlADZ(control, value, adz);
        if (alt) {
            eControl newControl;
            switch (control) {
                case ControlVehicleAccelerate:
                    newControl = (ControlVehicleSubThrottleUp);
                    break;
                case ControlVehicleBrake:
                    newControl = (ControlVehicleSubThrottleDown);
                    break;
                case ControlVehicleMoveLeftRight:
                    newControl = (ControlVehicleSubTurnLeftRight);
                    break;
                default:
                    newControl = (control);
                    break;
            }

            Controls::SetControlADZ(newControl, value, adz);
        }
    }
}

// Check for "alternative" movement methods for cars.
// Applies to:
//   - Amphibious when wet
//   - Flying when in-air
bool hasAltInputs(Vehicle vehicle) {
    Hash vehicleModel = ENTITY::GET_ENTITY_MODEL(vehicle);

    bool allWheelsOnGround = VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(vehicle);
    float submergeLevel = ENTITY::GET_ENTITY_SUBMERGED_LEVEL(vehicle);
    // 5: Stromberg
    // 6: Amphibious cars / APC
    // 7: Amphibious bike
    int modelType = g_ext.GetModelType(vehicle);
    bool isFrog = modelType == 5 || modelType == 6 || modelType == 7;

    float hoverRatio = g_ext.GetHoverTransformRatio(vehicle);

    bool altInput;

    // Wet amphibious vehicles
    if (isFrog && submergeLevel > 0.0f) {
        altInput = true;
    }
    // Hovering
    else if (hoverRatio > 0.0f) {
        altInput = true;
    }
    // Everything else
    else {
        altInput = false;
    }

    return altInput;
}

///////////////////////////////////////////////////////////////////////////////
//                   Mod functions: Reverse/Pedal handling
///////////////////////////////////////////////////////////////////////////////

// Forward gear: Throttle accelerates, Brake brakes (exclusive)
// Reverse gear: Throttle reverses, Brake brakes (exclusive)
void WheelInput::HandlePedals(float wheelThrottleVal, float wheelBrakeVal) {
    bool altInput = hasAltInputs(g_playerVehicle);

    wheelThrottleVal = pow(wheelThrottleVal, g_settings.Wheel.Throttle.Gamma);
    wheelBrakeVal = pow(wheelBrakeVal, g_settings.Wheel.Brake.Gamma);

    float speedThreshold = 0.5f;
    const float reverseThreshold = 2.0f;

    if (g_vehData.mGearCurr > 0) {
        // Going forward
        if (g_vehData.mVelocity.y >= speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are going forward");
            // Throttle Pedal normal
            if (wheelThrottleVal > 0.01f) {
                Controls::SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, g_settings.Wheel.Throttle.AntiDeadZone);
            }
            // Brake Pedal normal
            if (wheelBrakeVal > 0.01f) {
                Controls::SetControlADZ(ControlVehicleBrake, wheelBrakeVal, g_settings.Wheel.Brake.AntiDeadZone);
            }
        }

        // Standing still
        if (abs(g_vehData.mVelocity.y) < speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are stopped");
            // Throttle Pedal normal
            if (wheelThrottleVal > 0.01f) {
                Controls::SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, g_settings.Wheel.Throttle.AntiDeadZone);
            }
            // Brake Pedal conditional
            if (wheelBrakeVal > wheelThrottleVal) {
                //showText(0.3, 0.0, 1.0, "We are stopped + just brake lights");
                
                if (wheelBrakeVal > 0.01f) {
                    //g_ext.SetThrottleP(g_playerVehicle, 0.1f);
                    g_ext.SetBrakeP(g_playerVehicle, 1.0f);
                    VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(g_playerVehicle, true);
                }
            }
            else {
                //showText(0.3, 0.0, 1.0, "We are stopped + burnout");
                // Brake Pedal normal
                if (wheelBrakeVal > 0.01f) {
                    Controls::SetControlADZ(ControlVehicleBrake, wheelBrakeVal, g_settings.Wheel.Brake.AntiDeadZone);
                }
            }
        }

        // Rolling back
        if (g_vehData.mVelocity.y <= -speedThreshold) {
            bool brakelights = false;
            // Just brake
            if (wheelThrottleVal <= 0.01f && wheelBrakeVal > 0.01f) {
                //showText(0.3, 0.0, 1.0, "We should brake");
                //showText(0.3, 0.05, 1.0, ("Brake pressure:" + std::to_string(wheelBrakeVal)));
                Controls::SetControlADZ(ControlVehicleAccelerate, wheelBrakeVal, g_settings.Wheel.Brake.AntiDeadZone);
                g_ext.SetThrottleP(g_playerVehicle, 0.1f);
                g_ext.SetBrakeP(g_playerVehicle, 1.0f);
                brakelights = true;
            }

            if (!g_gearStates.FakeNeutral && wheelThrottleVal > 0.01f && !isClutchPressed()) {
                //showText(0.3, 0.0, 1.0, "We should burnout");
                if (g_controls.BrakeVal < 0.1f) {
                    VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(g_playerVehicle, false);
                }
                for (int i = 0; i < g_vehData.mWheelCount; i++) {
                    if (g_ext.IsWheelPowered(g_playerVehicle, i)) {
                        g_ext.SetWheelBrakePressure(g_playerVehicle, i, 0.0f);
                        g_ext.SetWheelPower(g_playerVehicle, i, 2.0f * g_ext.GetDriveForce(g_playerVehicle));
                    }
                    else {
                        float handlingBrakeForce = *reinterpret_cast<float*>(g_vehData.mHandlingPtr + hOffsets.fBrakeForce);
                        float inpBrakeForce = handlingBrakeForce * g_controls.BrakeVal;
                        g_ext.SetWheelPower(g_playerVehicle, i, 0.0f);
                        g_ext.SetWheelBrakePressure(g_playerVehicle, i, inpBrakeForce);
                    }
                }
                fakeRev();
                g_ext.SetThrottle(g_playerVehicle, g_controls.ThrottleVal);
                g_ext.SetThrottleP(g_playerVehicle, g_controls.ThrottleVal);
                g_wheelPatchStates.InduceBurnout = true;
            }

            if (wheelThrottleVal > 0.01f && (isClutchPressed() || g_gearStates.FakeNeutral)) {
                if (wheelBrakeVal > 0.01f) {
                    //showText(0.3, 0.0, 1.0, "We should rev and brake");
                    //showText(0.3, 0.05, 1.0, ("Brake pressure:" + std::to_string(wheelBrakeVal)) );
                    Controls::SetControlADZ(ControlVehicleAccelerate, wheelBrakeVal, g_settings.Wheel.Brake.AntiDeadZone);
                    g_ext.SetThrottleP(g_playerVehicle, 0.1f);
                    g_ext.SetBrakeP(g_playerVehicle, 1.0f);
                    brakelights = true;
                    fakeRev(false, 0);
                }
                else if (g_controls.ClutchVal > 0.9 || g_gearStates.FakeNeutral) {
                    //showText(0.3, 0.0, 1.0, "We should rev and do nothing");
                    g_ext.SetThrottleP(g_playerVehicle, wheelThrottleVal);
                    fakeRev(false, 0);
                }
                else {
                    //showText(0.3, 0.0, 1.0, "We should rev and apply throttle");
                    Controls::SetControlADZ(ControlVehicleAccelerate, wheelThrottleVal, g_settings.Wheel.Throttle.AntiDeadZone);
                    g_ext.SetThrottleP(g_playerVehicle, wheelThrottleVal);
                    fakeRev(false, 0);
                }
            }
            VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(g_playerVehicle, brakelights);
        }
        else {
            g_wheelPatchStates.InduceBurnout = false;
        }
    }

    if (g_vehData.mGearCurr == 0) {
        // Enables reverse lights
        g_ext.SetThrottleP(g_playerVehicle, -0.1f);

        // We're reversing
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true).y < -speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are reversing");
            // Throttle Pedal Reverse
            if (wheelThrottleVal > 0.01f) {
                Controls::SetControlADZ(ControlVehicleBrake, wheelThrottleVal, g_settings.Wheel.Throttle.AntiDeadZone);
            }
            // Brake Pedal Reverse
            if (wheelBrakeVal > 0.01f) {
                Controls::SetControlADZ(ControlVehicleAccelerate, wheelBrakeVal, g_settings.Wheel.Brake.AntiDeadZone);
                g_ext.SetThrottleP(g_playerVehicle, -wheelBrakeVal);
                g_ext.SetBrakeP(g_playerVehicle, 1.0f);
            }
        }

        // Standing still
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true).y < speedThreshold && ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true).y >= -speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are stopped");

            if (wheelThrottleVal > 0.01f) {
                Controls::SetControlADZ(ControlVehicleBrake, wheelThrottleVal, g_settings.Wheel.Throttle.AntiDeadZone);
            }

            if (wheelBrakeVal > 0.01f) {
                g_ext.SetThrottleP(g_playerVehicle, -wheelBrakeVal);
                g_ext.SetBrakeP(g_playerVehicle, 1.0f);
                VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(g_playerVehicle, true);
            }
        }

        // We're rolling forwards
        if (ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true).y > speedThreshold) {
            //showText(0.3, 0.0, 1.0, "We are rolling forwards");
            //bool brakelights = false;

            if (ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true).y > reverseThreshold) {
                if (!isClutchPressed()) {
                    CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, 1.0f);
                }
                // Brake Pedal Reverse
                if (wheelBrakeVal > 0.01f) {
                    Controls::SetControlADZ(ControlVehicleBrake, wheelBrakeVal, g_settings.Wheel.Brake.AntiDeadZone);
                    g_ext.SetThrottleP(g_playerVehicle, -wheelBrakeVal);
                    g_ext.SetBrakeP(g_playerVehicle, 1.0f);
                }
            }

            //VEHICLE::SET_VEHICLE_BRAKE_LIGHTS(vehicle, brakelights);
        }
    }
}

// Pedals behave like RT/LT
void WheelInput::HandlePedalsArcade(float wheelThrottleVal, float wheelBrakeVal) {
    bool altInput = hasAltInputs(g_playerVehicle);
    wheelThrottleVal = pow(wheelThrottleVal, g_settings.Wheel.Throttle.Gamma);
    wheelBrakeVal = pow(wheelBrakeVal, g_settings.Wheel.Brake.Gamma);

    if (wheelThrottleVal > 0.01f) {
        SetControlADZAlt(ControlVehicleAccelerate, wheelThrottleVal, g_settings.Wheel.Throttle.AntiDeadZone, altInput);
    }
    if (wheelBrakeVal > 0.01f) {
        SetControlADZAlt(ControlVehicleBrake, wheelBrakeVal, g_settings.Wheel.Brake.AntiDeadZone, altInput);
    }
}


///////////////////////////////////////////////////////////////////////////////
//                       Mod functions: Buttons
///////////////////////////////////////////////////////////////////////////////

// Set state, and activate them if hazards are off.
void setBlinkers(bool left, bool right) {
    g_peripherals.BlinkerLeft = left;
    g_peripherals.BlinkerRight = right;

    if (!g_peripherals.BlinkerHazard) {
        VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, 0, right);
        VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, 1, left);
    }
}

void checkIndicatorActions() {
    if (g_controls.ButtonJustPressed(CarControls::WheelControlType::IndicatorHazard)) {
        if (!g_peripherals.BlinkerHazard) {
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, 0, true);
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, 1, true);
            g_peripherals.BlinkerHazard = true;
        }
        else {
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, 0, g_peripherals.BlinkerRight);
            VEHICLE::SET_VEHICLE_INDICATOR_LIGHTS(g_playerVehicle, 1, g_peripherals.BlinkerLeft);
            g_peripherals.BlinkerHazard = false;
        }
    }

    if (g_controls.ButtonJustPressed(CarControls::WheelControlType::IndicatorLeft)) {
        if (!g_peripherals.BlinkerLeft) {
            g_peripherals.BlinkerTicks = 1;
            setBlinkers(true, false);
        }
        else {
            g_peripherals.BlinkerTicks = 0;
            setBlinkers(false, false);
        }
    }
    if (g_controls.ButtonJustPressed(CarControls::WheelControlType::IndicatorRight)) {
        if (!g_peripherals.BlinkerRight) {
            g_peripherals.BlinkerTicks = 1;
            setBlinkers(false, true);
        }
        else {
            g_peripherals.BlinkerTicks = 0;
            setBlinkers(false, false);
        }
    }

    float wheelCenterDeviation = (g_controls.SteerVal - 0.5f) / 0.5f;

    if (g_peripherals.BlinkerTicks == 1 && abs(wheelCenterDeviation) > 0.2f) {
        g_peripherals.BlinkerTicks = 2;
    }

    if (g_peripherals.BlinkerTicks == 2 && abs(wheelCenterDeviation) < 0.1f) {
        g_peripherals.BlinkerTicks = 0;
        setBlinkers(false, false);
    }
}

void checkRadioButtons() {
    if (g_controls.ButtonHeld(CarControls::WheelControlType::RadioPrev, 1000) ||
        g_controls.ButtonHeld(CarControls::WheelControlType::RadioNext, 1000)) {
        if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() != RadioOff) {
            g_peripherals.RadioStationIndex = AUDIO::GET_PLAYER_RADIO_STATION_INDEX();
        }
        AUDIO::SET_VEH_RADIO_STATION(g_playerVehicle, "OFF");
        return;
    }
    if (g_controls.ButtonReleased(CarControls::WheelControlType::RadioNext)) {
        if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() == RadioOff) {
            AUDIO::SET_RADIO_TO_STATION_INDEX(g_peripherals.RadioStationIndex);
            return;
        }
        AUDIO::_0xFF266D1D0EB1195D(); // Next radio station
    }
    if (g_controls.ButtonReleased(CarControls::WheelControlType::RadioPrev)) {
        if (AUDIO::GET_PLAYER_RADIO_STATION_INDEX() == RadioOff) {
            AUDIO::SET_RADIO_TO_STATION_INDEX(g_peripherals.RadioStationIndex);
            return;
        }
        AUDIO::_0xDD6BCF9E94425DF9(); // Prev radio station
    }
}

void checkCameraButtons() {
    if (g_controls.ButtonIn(CarControls::WheelControlType::LookBack)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleLookBehind, 1.0f);
    }

    // who was first?
    if (g_controls.ButtonIn(CarControls::WheelControlType::LookRight) &&
        g_controls.ButtonJustPressed(CarControls::WheelControlType::LookLeft)) {
        g_peripherals.LookBackRShoulder = true;
    }

    if (g_controls.ButtonIn(CarControls::WheelControlType::LookLeft) &&
        g_controls.ButtonIn(CarControls::WheelControlType::LookRight)) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(g_peripherals.LookBackRShoulder ? -180.0f : 180.0f);
    }
    else if (g_controls.ButtonIn(CarControls::WheelControlType::LookLeft)) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_PITCH(0.0f, 1.0f);
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(90.0f);
    }
    else if (g_controls.ButtonIn(CarControls::WheelControlType::LookRight)) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_PITCH(0.0f, 1.0f);
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(-90.0f);
    }
    if (g_controls.ButtonReleased(CarControls::WheelControlType::LookLeft) && !g_controls.ButtonIn(CarControls::WheelControlType::LookRight) ||
        g_controls.ButtonReleased(CarControls::WheelControlType::LookRight) && !g_controls.ButtonIn(CarControls::WheelControlType::LookLeft)) {
        CAM::SET_GAMEPLAY_CAM_RELATIVE_HEADING(0.0f);
    }
    if (g_controls.ButtonReleased(CarControls::WheelControlType::LookLeft) ||
        g_controls.ButtonReleased(CarControls::WheelControlType::LookRight)) {
        g_peripherals.LookBackRShoulder = false;
    }

    if (g_controls.ButtonJustPressed(CarControls::WheelControlType::Camera)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlNextCamera, 1.0f);
    }
}

void checkVehicleInputButtons() {
    if (g_controls.HandbrakeVal > 0.1f) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, g_controls.HandbrakeVal);
    }
    if (g_controls.ButtonIn(CarControls::WheelControlType::Handbrake)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHandbrake, 1.0f);
    }
    if (g_controls.ButtonIn(CarControls::WheelControlType::Horn)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHorn, 1.0f);
    }
    if (g_controls.ButtonJustPressed(CarControls::WheelControlType::Lights)) {
        CONTROLS::_SET_CONTROL_NORMAL(0, ControlVehicleHeadlight, 1.0f);
    }
}

void checkAssistButtons() {
    if (g_controls.ButtonJustPressed(CarControls::WheelControlType::ToggleABS)) {
        APPLY_CONFIG_VALUE(DriveAssists.ABS.Enable, !g_settings().DriveAssists.ABS.Enable);
    }
    if (g_controls.ButtonJustPressed(CarControls::WheelControlType::ToggleESC)) {
        APPLY_CONFIG_VALUE(DriveAssists.ESP.Enable, !g_settings().DriveAssists.ESP.Enable);
    }
    if (g_controls.ButtonJustPressed(CarControls::WheelControlType::ToggleTCS)) {
        APPLY_CONFIG_VALUE(DriveAssists.TCS.Enable, !g_settings().DriveAssists.TCS.Enable);
    }
}

void WheelInput::CheckButtons() {
    if (g_controls.PrevInput != CarControls::Wheel) {
        return;
    }

    checkAssistButtons();
    checkVehicleInputButtons();
    checkCameraButtons();
    checkRadioButtons();
    checkIndicatorActions();
}

///////////////////////////////////////////////////////////////////////////////
//                    Wheel input and force feedback
///////////////////////////////////////////////////////////////////////////////

void WheelInput::DoSteering() {
    if (g_controls.PrevInput != CarControls::Wheel)
        return;

    float steerMult;
    if (g_vehData.mClass == VehicleClass::Bike || g_vehData.mClass == VehicleClass::Quad)
        steerMult = g_settings.Wheel.Steering.AngleMax / g_settings().Wheel.Steering.AngleBike;
    else if (g_vehData.mClass == VehicleClass::Car)
        steerMult = g_settings.Wheel.Steering.AngleMax / g_settings().Wheel.Steering.AngleCar;
    else {
        steerMult = g_settings.Wheel.Steering.AngleMax / g_settings().Wheel.Steering.AngleBoat;
    }

    float steerClamp = g_settings.Wheel.Steering.AngleMax / steerMult;

    float steerValL = map(g_controls.SteerVal, 0.0f, 0.5f, 1.0f, 0.0f);
    float steerValR = map(g_controls.SteerVal, 0.5f, 1.0f, 0.0f, 1.0f);
    float steerValGammaL = pow(steerValL, g_settings.Wheel.Steering.Gamma);
    float steerValGammaR = pow(steerValR, g_settings.Wheel.Steering.Gamma);
    float steerValGamma = g_controls.SteerVal < 0.5f ? -steerValGammaL : steerValGammaR;
    float effSteer = steerMult * steerValGamma;

    bool altInputs = hasAltInputs(g_playerVehicle);

    if (g_vehData.mClass == VehicleClass::Car) {
        g_ext.SetSteeringInputAngle(g_playerVehicle, -effSteer);

        auto boneIdx = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(g_playerVehicle, "steeringwheel");
        if (boneIdx != -1 && g_settings.Wheel.Options.SyncRotation) {
            Vector3 rotAxis{};
            rotAxis.y = 1.0f;
            float rotDeg = g_settings().Wheel.Steering.AngleMax / 2.0f * steerValGamma;

            // clamp
            if (abs(rotDeg) > steerClamp / 2.0f) {
                rotDeg = std::clamp(rotDeg, -steerClamp / 2.0f, steerClamp / 2.0f);
            }
            float rotDegRaw = rotDeg;
            // Setting angle using the g_ext calls above causes the angle to overshoot the "real" coords
            // Not sure if this is the best solution, but hey, it works!
            rotDeg -= 2.0f * rad2deg(std::clamp(effSteer, -1.0f, 1.0f) * g_ext.GetMaxSteeringAngle(g_playerVehicle));

            VehicleBones::RotateAxis(g_playerVehicle, boneIdx, rotAxis, rotDeg);
            SteeringAnimation::SetRotation(rotDegRaw);
        }
    }
    if (g_vehData.mClass != VehicleClass::Car || altInputs){
        SetControlADZAlt(ControlVehicleMoveLeftRight, effSteer, g_settings.Wheel.Steering.AntiDeadZone, altInputs);
    }
}

int calculateDamper(float gain, float wheelsOffGroundRatio) {
    Vector3 accelValsAvg = g_vehData.mAcceleration; //TODO: Avg/dampen later

    // Just to use floats everywhere here
    float damperMax = static_cast<float>(g_settings().Wheel.FFB.DamperMax);
    float damperMin = static_cast<float>(g_settings().Wheel.FFB.DamperMin);
    float damperMinSpeed = static_cast<float>(g_settings().Wheel.FFB.DamperMinSpeed);

    float absVehicleSpeed = abs(ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true).y);
    float damperFactorSpeed = map(absVehicleSpeed, 0.0f, damperMinSpeed, damperMax, damperMin);
    damperFactorSpeed = fmaxf(damperFactorSpeed, damperMin);
    // already clamped on the upper bound by abs(vel) in map()

    // 2G of deceleration causes addition of damperMin. Heavier feeling steer when braking!
    float damperFactorAccel = map(-accelValsAvg.y / 9.81f, -2.0f, 2.0f, -damperMin, damperMin);
    damperFactorAccel = fmaxf(damperFactorAccel, -damperMin);
    damperFactorAccel = fminf(damperFactorAccel, damperMin);

    float damperForce = damperFactorSpeed + damperFactorAccel;

    damperForce = damperForce * (1.0f - wheelsOffGroundRatio);

    if (g_vehData.mClass == VehicleClass::Car || g_vehData.mClass == VehicleClass::Quad) {
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(g_playerVehicle, 0, true)) {
            damperForce /= 2.0f;
        }
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(g_playerVehicle, 1, true)) {
            damperForce /= 2.0f;
        }
    }
    else if (g_vehData.mClass == VehicleClass::Bike) {
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(g_playerVehicle, 0, true)) {
            damperForce /= 4.0f;
        }
    }

    if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(g_playerVehicle)) {
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
    auto compSpeed = g_vehData.mSuspensionTravelSpeeds;

    // More than 2 wheels! Trikes should be ok, etc.
    if (compSpeed.size() > 2) {
        // left should pull left, right should pull right
        compSpeedTotal = -compSpeed[0] + compSpeed[1];
    }

    return static_cast<int>(1000.0f * g_settings().Wheel.FFB.DetailMult * compSpeedTotal);
}

void calculateSoftLock(int& totalForce, int& damperForce) {
    float steerMult;
    if (g_vehData.mClass == VehicleClass::Bike || g_vehData.mClass == VehicleClass::Quad)
        steerMult = g_settings.Wheel.Steering.AngleMax / g_settings().Wheel.Steering.AngleBike;
    else if (g_vehData.mClass == VehicleClass::Car)
        steerMult = g_settings.Wheel.Steering.AngleMax / g_settings().Wheel.Steering.AngleCar;
    else {
        steerMult = g_settings.Wheel.Steering.AngleMax / g_settings().Wheel.Steering.AngleBoat;
    }
    float effSteer = steerMult * 2.0f * (g_controls.SteerVal - 0.5f);
    float steerSpeed = g_controls.GetAxisSpeed(CarControls::WheelAxisType::Steer);
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
    float speed = ENTITY::GET_ENTITY_SPEED(g_playerVehicle);

    const float maxSpeed = g_settings.Wheel.FFB.MaxSpeed;
    // gamma: should be < 1 for tapering off force when reaching maxSpeed
    //float spdMap = pow(std::min(speed, maxSpeed) / maxSpeed, g_settings.Wheel.FFB.Gamma) * maxSpeed / 2.0f;
    float spdMap = pow(std::min(speed, maxSpeed * 2.0f) / (maxSpeed * 2.0f), g_settings.Wheel.FFB.Gamma) * maxSpeed;
    float spdRatio = spdMap / speed;

    if (speed == 0.0f)
        spdRatio = 1.0f;

    Vector3 speedVector = ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true);
    Vector3 speedVectorMapped = speedVector;
    speedVectorMapped.x = speedVector.x * (spdRatio);
    Vector3 rotVector = ENTITY::GET_ENTITY_ROTATION_VELOCITY(g_playerVehicle);
    Vector3 rotRelative{
        speed * -sin(rotVector.z), 0,
        speed * cos(rotVector.z), 0,
        0, 0
    };

    Vector3 expectedVectorMapped{
        spdMap * -sin(steeringAngle / g_settings.Wheel.Steering.SteerMult), 0,
        spdMap * cos(steeringAngle / g_settings.Wheel.Steering.SteerMult), 0,
        0, 0
    };

    Vector3 expectedVector{
        speed * -sin(steeringAngle / g_settings.Wheel.Steering.SteerMult), 0,
        speed * cos(steeringAngle / g_settings.Wheel.Steering.SteerMult), 0,
        0, 0
    };
    
    float error = static_cast<float>(pid.getOutput(expectedVectorMapped.x, static_cast<double>(speedVectorMapped.x) * g_settings().Wheel.FFB.SATFactor));

    int satForce = static_cast<int>(g_settings().Wheel.FFB.SATAmpMult * static_cast<float>(defaultGain) * -error);

    // "Reduction" effects - those that affect already calculated things
    bool understeering = false;
    float understeer = 0.0f;

    // understeer
    if (isCar)  {
        float avgAngle = g_ext.GetWheelAverageAngle(g_playerVehicle) * g_settings.Wheel.Steering.SteerMult;

        Vector3 vecPredStr{
            speed * -sin(avgAngle / g_settings.Wheel.Steering.SteerMult), 0,
            speed * cos(avgAngle / g_settings.Wheel.Steering.SteerMult), 0,
            0, 0
        };

        Vector3 vecNextSpd = ENTITY::GET_ENTITY_SPEED_VECTOR(g_playerVehicle, true);
        Vector3 vecNextRot = (vecNextSpd + rotRelative) * 0.5f;
        float understeerAngle = GetAngleBetween(vecNextRot, vecPredStr);

        float dSpdStr = Distance(vecNextSpd, vecPredStr);
        float aSpdRot = GetAngleBetween(vecNextSpd, vecNextRot);

        float dSpdRot = Distance(vecNextSpd, vecNextRot);
        float aSpdStr = GetAngleBetween(vecNextSpd, vecPredStr);

        if (dSpdStr > dSpdRot&& sgn(aSpdRot) == sgn(aSpdStr)) {
            if (g_vehData.mVelocity.y > 10.0f && abs(avgAngle) > deg2rad(2.0f)) {
                understeer = sgn(speedVector.x - expectedVector.x) * (vecNextRot.x - expectedVector.x);
                understeering = true;
                satForce = static_cast<int>(static_cast<float>(satForce) / std::max(1.0f, 3.3f * understeer + 1.0f));
            }
        }
    }

    satForce = static_cast<int>(static_cast<float>(satForce) * (1.0f - wheelsOffGroundRatio));

    if (g_vehData.mClass == VehicleClass::Car || g_vehData.mClass == VehicleClass::Quad) {
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(g_playerVehicle, 0, true)) {
            satForce = satForce / 4;
        }
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(g_playerVehicle, 1, true)) {
            satForce = satForce / 4;
        }
    }
    else if (g_vehData.mClass == VehicleClass::Bike) {
        if (VEHICLE::IS_VEHICLE_TYRE_BURST(g_playerVehicle, 0, true)) {
            satForce = satForce / 10;
        }
    }

    if (g_settings.Debug.DisplayInfo) {
        //showText(0.85, 0.175, 0.4, fmt::format("RelSteer:\t{:.3f}", steeringRelative.x), 4);
        //showText(0.85, 0.200, 0.4, fmt::format("SetPoint:\t{:.3f}", travelRelative.x), 4);
        showText(0.85, 0.225, 0.4, fmt::format("Error:\t\t{:.3f}", error), 4);
        showText(0.85, 0.250, 0.4, fmt::format("{}Under:\t\t{:.3f}~w~", understeering ? "~b~" : "~w~", understeer), 4);
    }
    if (satForce > 0) {
        satForce = map(satForce, 0, 10000, g_settings().Wheel.FFB.AntiDeadForce, 10000);
    }
    if (satForce < 0) {
        satForce = map(satForce, -10000, 0, -10000, -g_settings().Wheel.FFB.AntiDeadForce);
    }
    satForce = std::clamp(satForce, -g_settings().Wheel.FFB.SATMax, g_settings().Wheel.FFB.SATMax);

    return satForce;
}

// Despite being scientifically inaccurate, "self-aligning torque" is the best description.
void WheelInput::DrawDebugLines() {
    float steeringAngle = g_ext.GetWheelAverageAngle(g_playerVehicle) * g_ext.GetSteeringMultiplier(g_playerVehicle);
    Vector3 velocityWorld = ENTITY::GET_ENTITY_VELOCITY(g_playerVehicle);
    Vector3 positionWorld = ENTITY::GET_ENTITY_COORDS(g_playerVehicle, 1);
    Vector3 travelWorld = velocityWorld + positionWorld;
    Vector3 travelRelative = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(g_playerVehicle, travelWorld.x, travelWorld.y, travelWorld.z);

    Vector3 rotationVelocity = ENTITY::GET_ENTITY_ROTATION_VELOCITY(g_playerVehicle);
    Vector3 turnWorld = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(g_playerVehicle, ENTITY::GET_ENTITY_SPEED(g_playerVehicle) * -sin(rotationVelocity.z), ENTITY::GET_ENTITY_SPEED(g_playerVehicle) * cos(rotationVelocity.z), 0.0f);
    Vector3 turnRelative = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(g_playerVehicle, turnWorld.x, turnWorld.y, turnWorld.z);
    float turnRelativeNormX = (travelRelative.x + turnRelative.x) / 2.0f;
    float turnRelativeNormY = (travelRelative.y + turnRelative.y) / 2.0f;
    Vector3 turnWorldNorm = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(g_playerVehicle, turnRelativeNormX, turnRelativeNormY, 0.0f);

    float steeringAngleRelX = ENTITY::GET_ENTITY_SPEED(g_playerVehicle) * -sin(steeringAngle);
    float steeringAngleRelY = ENTITY::GET_ENTITY_SPEED(g_playerVehicle) * cos(steeringAngle);
    Vector3 steeringWorld = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(g_playerVehicle, steeringAngleRelX, steeringAngleRelY, 0.0f);

    GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, travelWorld.x, travelWorld.y, travelWorld.z, 0, 255, 0, 255);
    GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, turnWorldNorm.x, turnWorldNorm.y, turnWorldNorm.z, 255, 0, 0, 255);
    GRAPHICS::DRAW_LINE(positionWorld.x, positionWorld.y, positionWorld.z, steeringWorld.x, steeringWorld.y, steeringWorld.z, 255, 0, 255, 255);
}

float getFloatingSteeredWheelsRatio(Vehicle v) {
    auto suspensionStates = g_vehData.mWheelsOnGround;
    auto angles = g_vehData.mWheelSteeringAngles;

    float wheelsOffGroundRatio = 0.0f;
    float wheelsInAir = 0.0f;
    float wheelsSteered = 0.0f;

    for (int i = 0; i < g_vehData.mWheelCount; i++) {
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
    switch (g_vehData.mClass) {
    case VehicleClass::Bike:
    case VehicleClass::Quad:
    case VehicleClass::Bicycle:
        return g_settings().Wheel.Steering.AngleBike;
    case VehicleClass::Car:
        return g_settings().Wheel.Steering.AngleCar;
    case VehicleClass::Boat:
        return g_settings().Wheel.Steering.AngleBoat;
    case VehicleClass::Plane:
    case VehicleClass::Heli:
    case VehicleClass::Train:
    case VehicleClass::Unknown:
    default: return g_settings.Wheel.Steering.AngleCar;
    }
}

void WheelInput::PlayFFBGround() {
    if (!g_settings().Wheel.FFB.Enable ||
        g_controls.PrevInput != CarControls::Wheel) {
        return;
    }

    float rotationScale = 1.0f;
    if (g_settings().Wheel.FFB.Scale) {
        rotationScale = getSteeringLock() / g_settings.Wheel.Steering.AngleMax * 0.66f + 0.34f;
    }

    if (g_settings.Wheel.Options.LogiLEDs) {
        g_controls.PlayLEDs(g_vehData.mRPM, 0.45f, 0.95f);
    }

    // avgAngle: left is positive
    // steerVal: left is negative
    // Rear-wheel steered cars don't match, so this needs to be flipped in that case.
    float avgAngle = g_ext.GetWheelAverageAngle(g_playerVehicle) * g_settings.Wheel.Steering.SteerMult;

    float steerVal = map(g_controls.SteerVal, 0.0f, 1.0f, -1.0f, 1.0f);    
    if (sgn(avgAngle) == sgn(steerVal)) {
        avgAngle = -avgAngle;
    }

    float wheelsOffGroundRatio = getFloatingSteeredWheelsRatio(g_playerVehicle);

    int detailForce = std::clamp(calculateDetail(), -g_settings().Wheel.FFB.DetailLim, g_settings().Wheel.FFB.DetailLim);
    int satForce = calculateSat(2500, avgAngle, wheelsOffGroundRatio, g_vehData.mClass == VehicleClass::Car);
    int damperForce = calculateDamper(50.0f, wheelsOffGroundRatio);

    // Decrease damper if sat rises, so constantForce doesn't fight against damper
    float damperMult = 1.0f - std::min(fabs((float)satForce), 10000.0f) / 10000.0f;
    damperForce = (int)(damperMult * (float)damperForce);

    // Dampen suspension, minimize damper, minimize SAT
    if (hasAltInputs(g_playerVehicle)) {
        detailForce /= 10;
        satForce /= 5;
        damperForce = g_settings().Wheel.FFB.DamperMin;
    }

    int totalForce = satForce + detailForce;
    totalForce = (int)((float)totalForce * rotationScale);
    calculateSoftLock(totalForce, damperForce);

    g_controls.PlayFFBDynamics(std::clamp(totalForce, -10000, 10000), std::clamp(damperForce, -10000, 10000));

    const float minGforce = 5.0f;
    const float maxGforce = 50.0f;
    const float minForce = 500.0f;
    const float maxForce = 10000.0f;
    float gForce = abs(g_vehData.mAcceleration.y) / 9.81f; // TODO: Average/dampen later
    bool collision = gForce > minGforce;
    int res = static_cast<int>(map(gForce, minGforce, maxGforce, minForce, maxForce) * g_settings().Wheel.FFB.CollisionMult);
    if (collision) {
        g_controls.PlayFFBCollision(std::clamp(res, -10000, 10000));
    }

    if (collision) {
        UI::Notify(DEBUG, fmt::format("Collision @ ~r~{:.3f}G~w~~n~"
            "FFB: {}", gForce, res));
    }

    if (g_settings.Debug.DisplayInfo) {
        showText(0.85, 0.275, 0.4, fmt::format("{}FFBSat:\t\t{}~w~", abs(satForce) > g_settings.Wheel.FFB.SATMax ? "~r~" : "~w~", satForce), 4);
        showText(0.85, 0.300, 0.4, fmt::format("{}FFBFin:\t\t{}~w~", abs(totalForce) > 10000 ? "~r~" : "~w~", totalForce), 4);
        showText(0.85, 0.325, 0.4, fmt::format("Damper:\t\t{}", damperForce), 4);
        showText(0.85, 0.350, 0.4, fmt::format("Detail:\t\t{}", detailForce), 4);
    }
}

void WheelInput::PlayFFBWater() {
    if (!g_settings().Wheel.FFB.Enable ||
        g_controls.PrevInput != CarControls::Wheel) {
        return;
    }

    float rotationScale = 1.0f;
    if (g_settings().Wheel.FFB.Scale) {
        rotationScale = getSteeringLock() / g_settings.Wheel.Steering.AngleMax * 0.66f + 0.34f;
    }

    if (g_settings.Wheel.Options.LogiLEDs) {
        g_controls.PlayLEDs(g_vehData.mRPM, 0.45f, 0.95f);
    }

    auto suspensionStates = g_vehData.mWheelsOnGround;
    auto angles = g_vehData.mWheelSteeringAngles;

    bool isInWater = ENTITY::GET_ENTITY_SUBMERGED_LEVEL(g_playerVehicle) > 0.10f;
    int damperForce = calculateDamper(50.0f, isInWater ? 0.25f : 1.0f);
    int detailForce = calculateDetail();
    int satForce = calculateSat(750, ENTITY::GET_ENTITY_ROTATION_VELOCITY(g_playerVehicle).z, 1.0f, false);

    if (!isInWater) {
        satForce = 0;
    }

    int totalForce = satForce + detailForce;
    totalForce = (int)((float)totalForce * rotationScale);
    calculateSoftLock(totalForce, damperForce);
    g_controls.PlayFFBDynamics(totalForce, damperForce);

    if (g_settings.Debug.DisplayInfo) {
        showText(0.85, 0.275, 0.4, fmt::format("{}FFBSat:\t\t{}~w~", abs(satForce) > 10000 ? "~r~" : "~w~", satForce), 4);
        showText(0.85, 0.300, 0.4, fmt::format("{}FFBFin:\t\t{}~w~", abs(totalForce) > 10000 ? "~r~" : "~w~", totalForce), 4);
        showText(0.85, 0.325, 0.4, fmt::format("Damper:\t{}", damperForce), 4);
    }
}
