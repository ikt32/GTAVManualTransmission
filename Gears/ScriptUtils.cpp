#include "ScriptUtils.h"

#include "Util/UIUtils.h"
#include "Constants.h"
#include "fmt/format.h"

namespace {
    int notificationHandle = 0;
}

void UI::Notify(const std::string& message) {
    showNotification(fmt::format("{}\n{}", Constants::NotificationPrefix, message), &notificationHandle);
}
