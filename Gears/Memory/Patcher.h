#pragma once
#include <string>
#include <utility>
#include "PatternInfo.h"
#include "../Util/Logger.hpp"
#include "NativeMemory.hpp"
#include "../Util/Util.hpp"

namespace MemoryPatcher {
// simple NOP patcher?
class Patcher {
public:
    Patcher(std::string name, PatternInfo& pattern, bool verbose)
        : mName(std::move(name))
        , mPattern(pattern)
        , mVerbose(verbose)
        , mMaxAttempts(4)
        , mAttempts(0)
        , mPatched(false)
        , mAddress(0)
        , mTemp(0) { }

    Patcher(std::string name, PatternInfo& pattern) 
        : Patcher(name, pattern, false) { }

    virtual ~Patcher() = default;

    virtual bool Patch() {
        if (mAttempts > mMaxAttempts) {
            return false;
        }

        if (mVerbose) logger.Write(DEBUG, "PATCH: [%s] Patching", mName.c_str());

        if (mPatched) {
            if (mVerbose) logger.Write(DEBUG, "PATCH: [%s] Already patched", mName.c_str());
            return true;
        }

        mTemp = Apply();

        if (mTemp) {
            mAddress = mTemp;
            mPatched = true;
            mAttempts = 0;

            if (mVerbose) {
                std::string bytes = ByteArrayToString(mPattern.Data.data(), mPattern.Data.size());
                logger.Write(DEBUG, "PATCH: [%s] Found at @ 0x%p", mName.c_str(), mAddress);
                logger.Write(DEBUG, "PATCH: [%s] Patch success, original code: %s", mName.c_str(), bytes.c_str());
            }
            return true;
        }

        logger.Write(ERROR, "PATCH: [%s] Patch failed", mName.c_str());
        mAttempts++;

        if (mAttempts > mMaxAttempts) {
            logger.Write(ERROR, "PATCH: [%s] Patch attempt limit exceeded", mName.c_str());
            logger.Write(ERROR, "PATCH: [%s] Patching disabled", mName.c_str());
        }
        return false;
    }

    virtual bool Restore() {
        if (mVerbose)
            logger.Write(DEBUG, "PATCH: [%s] Restoring instructions", mName.c_str());

        if (!mPatched) {
            if (mVerbose)
                logger.Write(DEBUG, "PATCH: [%s] Already restored/intact", mName.c_str());
            return true;
        }

        if (mAddress) {
            memcpy((void*)mAddress, mPattern.Data.data(), mPattern.Data.size());
            mAddress = 0;
            mPatched = false;
            mAttempts = 0;

            if (mVerbose) {
                logger.Write(DEBUG, "PATCH: [%s] Restore success", mName.c_str());
            }
            return true;
        }

        logger.Write(ERROR, "PATCH: [%s] restore failed", mName.c_str());
        return false;
    }

    uintptr_t Test() const {
        auto addr = mem::FindPattern(mPattern.Pattern, mPattern.Mask);
        if (addr)
            logger.Write(DEBUG, "PATCH: Test: [%s] found at 0x%p", mName.c_str(), addr);
        else
            logger.Write(ERROR, "PATCH: Test: [%s] not found", mName.c_str());

        return addr;
    }

    bool Patched() const {
        return mPatched;
    }

protected:
    const std::string mName;
    PatternInfo& mPattern;
    bool mVerbose;

    const int mMaxAttempts;
    int mAttempts;
    bool mPatched;
    uintptr_t mAddress;
    uintptr_t mTemp;

    virtual uintptr_t Apply() {
        uintptr_t address;
        if (mTemp != NULL) {
            address = mTemp;
        }
        else {
            address = mem::FindPattern(mPattern.Pattern, mPattern.Mask);
            if (address) address += mPattern.Offset;
            logger.Write(DEBUG, "PATCH: [%s] Patched @ 0x%p", mName.c_str(), address);
        }

        if (address) {
            memcpy(mPattern.Data.data(), (void*)address, mPattern.Data.size());
            memset(reinterpret_cast<void *>(address), 0x90, mPattern.Data.size());
        }
        return address;
    }
};

// TODO: Can probably be removed when mPattern.Data makes more sense later?
class PatcherJmp : public Patcher {
public:
    PatcherJmp(const std::string& name, PatternInfo& pattern, bool verbose)
        : Patcher(name, pattern, verbose) {}

    PatcherJmp(const std::string& name, PatternInfo& pattern)
        : Patcher(name, pattern) {}

protected:
    uintptr_t Apply() override {
        uintptr_t address;
        if (mTemp != NULL) {
            address = mTemp;
        }
        else {
            address = mem::FindPattern(mPattern.Pattern, mPattern.Mask);
            if (address) address += mPattern.Offset;
            logger.Write(DEBUG, "PATCH: [%s] Patched @ 0x%p", mName.c_str(), address);
        }

        if (address) {
            uint8_t instrArr[6] =
            { 0xE9, 0x00, 0x00, 0x00, 0x00, 0x90 };               // make preliminary instruction: JMP to <adrr>
            uint8_t origSteerInstrDest[4] = { 0x00, 0x00, 0x00, 0x00 };

            memcpy(mPattern.Data.data(), (void*)address, 6); // save whole orig instruction
            memcpy(origSteerInstrDest, (void*)(address + 2), 4);    // save the address it writes to
            origSteerInstrDest[0] += 1;                             // Increment first byte by 1
            memcpy(instrArr + 1, origSteerInstrDest, 4);            // use saved address in new instruction
            memcpy((void*)address, instrArr, 6);                    // patch with new fixed instruction

            return address;
        }
        return 0;
    }
};
}
