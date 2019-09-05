#include "ScriptUtils.h"

#include "Constants.h"
#include "Util/MathExt.h"
#include "Util/UIUtils.h"
#include "inc/natives.h"
#include "fmt/format.h"


namespace {
    int notificationHandle = 0;
}

void UI::Notify(const std::string& message) {
    showNotification(fmt::format("{}\n{}", Constants::NotificationPrefix, message), &notificationHandle);
}

void Controls::SetControlADZ(eControl control, float value, float adz) {
    CONTROLS::_SET_CONTROL_NORMAL(0, control, sgn(value) * adz + (1.0f - adz) * value);
}
