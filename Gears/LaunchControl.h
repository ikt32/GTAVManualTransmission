#pragma once
namespace LaunchControl {
    enum class ELCState {
        Inactive,
        Staged,
        Limiting,
    };

    void Update(float& clutchVal);

    ELCState GetState();
};

