#include "SysUtils.h"
#include "Windows.h"

bool SysUtil::IsWindowFocused() {
    auto foregroundHwnd = GetForegroundWindow();
    DWORD foregroundProcId;
    GetWindowThreadProcessId(foregroundHwnd, &foregroundProcId);
    auto currentProcId = GetCurrentProcessId();
    return foregroundProcId == currentProcId;
}
