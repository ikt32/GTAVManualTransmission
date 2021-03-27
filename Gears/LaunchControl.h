#pragma once
namespace LaunchControl {
    enum class ELCState {
        Inactive,
        Staged,
        Limiting,
        Controlling,
    };

    void Update(float& clutchVal);

    ELCState GetState();
};

