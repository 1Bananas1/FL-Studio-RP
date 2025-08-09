#ifndef TRAY_H
#define TRAY_H

#include <string>
#include <windows.h>
#include <shellapi.h>
#include <functional>

class SystemTray {
    public:
        static const UINT ID_REFRESH_CONNECTION = 1001;
        static const UINT ID_DISCONNECT = 1002;
        static const UINT ID_EXIT = 1003;
        static const UINT WM_TRAY_ICON = WM_USER + 1;

    private:
        HWND m_hwnd;
        NOTIFYICONDATA m_notifyIconData;
        HMENU m_contextMenu;
        HICON m_icon;
        bool m_isVisible;

        // Callback functions
        std::function<void()> m_onRefreshConnection;
        std::function<void()> m_onDisconnect;
        std::function<void()> m_onExit;

        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        bool createWindow();
        bool createContextMenu();
        void showContextMenu();
        void handleMenuCommand(UINT menuId);

    public:
        SystemTray();
        ~SystemTray();
        bool initialize(const std::string& iconPath, const std::string& tooltip = "FL Studio Rich Presence");
        bool show();
        bool hide();
        bool updateTooltip(const std::string& tooltip);
        bool processMessages();
        void setRefreshConnectionCallback(std::function<void()> callback);
        void setDisconnectCallback(std::function<void()> callback);
        void setExitCallback(std::function<void()> callback);
        bool isVisible() const { return m_isVisible; }
};
#endif