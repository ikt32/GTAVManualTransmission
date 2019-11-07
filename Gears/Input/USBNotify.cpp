//https://stackoverflow.com/questions/16214768/detecting-usb-insertion-removal-in-c-non-gui-application

#define ANSI
#define _WIN32_WINNT   0x0501

#include "USBNotify.h"
#include "../Util/Util.hpp"
#include "../Util/Logger.hpp"

#include <windows.h>
#include <winuser.h>
#include <Dbt.h>

#include <string>
#include <functional>

#define HID_CLASSGUID {0x4d1e55b2, 0xf16f, 0x11cf,{ 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30}}
#define CLS_NAME "DUMMY_CLASS"
#define HWND_MESSAGE     ((HWND)-3)

std::function<void()> onConnect;

LRESULT message_handler(HWND__* hwnd, UINT uint, WPARAM wparam, LPARAM lparam) {
    switch (uint) {
        // before window creation
        case WM_NCCREATE: 
            return true;
            break;
        // the actual creation of the window
        case WM_CREATE: {
            // you can get your creation params here..like GUID..
            LPCREATESTRUCT params = (LPCREATESTRUCT)lparam;
            GUID InterfaceClassGuid = *((GUID*)params->lpCreateParams);
            DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
            ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
            NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
            NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
            NotificationFilter.dbcc_classguid = InterfaceClassGuid;
            HDEVNOTIFY dev_notify = RegisterDeviceNotification(hwnd, &NotificationFilter,
                DEVICE_NOTIFY_WINDOW_HANDLE);
            if (dev_notify == NULL) {
                logger.Write(ERROR, "[ USB ] Could not register for device notifications!");
            }
            break;
        }

        case WM_DEVICECHANGE: {
            PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lparam;
            PDEV_BROADCAST_DEVICEINTERFACE lpdbv = (PDEV_BROADCAST_DEVICEINTERFACE)lpdb;
            std::string path;
            if (lpdb->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
                path = std::string(StrUtil::utf8_encode(lpdbv->dbcc_name));
                switch (wparam) {
                    case DBT_DEVICEARRIVAL:
                        logger.Write(INFO, "[ USB ] New device connected: %s", path.c_str());
                        if (onConnect)
                            onConnect();
                        break;

                    case DBT_DEVICEREMOVECOMPLETE:
                        logger.Write(INFO, "[ USB ] Device disconnected: %s", path.c_str());
                        break;
                }
            }
            break;
        }
    }
    return 0L;
}

void USB::Init(const std::function<void()>& func) {
    HWND hWnd = NULL;
    WNDCLASSEX wx;
    ZeroMemory(&wx, sizeof(wx));

    wx.cbSize = sizeof(WNDCLASSEX);
    wx.lpfnWndProc = reinterpret_cast<WNDPROC>(message_handler);
    wx.hInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(0));
    wx.style = CS_HREDRAW | CS_VREDRAW;
    wx.hInstance = GetModuleHandle(0);
    wx.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wx.lpszClassName = StrUtil::utf8_decode(CLS_NAME).c_str();

    GUID guid = HID_CLASSGUID;

    if (RegisterClassEx(&wx)) {
        hWnd = CreateWindowA(CLS_NAME, "DevNotifWnd", WS_ICONIC,
            0, 0, CW_USEDEFAULT, 0, HWND_MESSAGE,
            NULL, GetModuleHandle(0), (void*)&guid);
    }

    if (hWnd == NULL) {
        logger.Write(ERROR, "[ USB ] Could not create message window!");
    }

    onConnect = func;
}

void USB::Update() {
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
