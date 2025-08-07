#include "tray.h"
#include <iostream>
#include <string>

// Window class name for the hidden window
static const wchar_t* WINDOW_CLASS_NAME = L"FLRPTrayWindow";

// Static pointer to the SystemTray instance for WindowProc
static SystemTray* g_trayInstance = nullptr;

SystemTray::SystemTray() 
    : m_hwnd(nullptr)
    , m_contextMenu(nullptr)
    , m_isVisible(false) {
    
    // Initialize NOTIFYICONDATA structure
    ZeroMemory(&m_notifyIconData, sizeof(NOTIFYICONDATA));
    m_notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    m_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_notifyIconData.uCallbackMessage = WM_TRAY_ICON;
    
    g_trayInstance = this;
}

SystemTray::~SystemTray() {
    hide();
    
    if (m_contextMenu) {
        DestroyMenu(m_contextMenu);
        m_contextMenu = nullptr;
    }
    
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    
    g_trayInstance = nullptr;
}

bool SystemTray::initialize(const std::string& iconPath, const std::string& tooltip) {
    // Create hidden window for message processing
    if (!createWindow()) {
        std::cerr << "Failed to create tray window" << std::endl;
        return false;
    }
    
    // Load icon from file
    HICON icon = (HICON)LoadImageA(nullptr, iconPath.c_str(), IMAGE_ICON, 
                                   GetSystemMetrics(SM_CXSMICON), 
                                   GetSystemMetrics(SM_CYSMICON), 
                                   LR_LOADFROMFILE);
    
    if (!icon) {
        std::cerr << "Failed to load icon from: " << iconPath << std::endl;
        return false;
    }
    
    // Setup tray icon data
    m_notifyIconData.hWnd = m_hwnd;
    m_notifyIconData.hIcon = icon;
    strcpy_s(m_notifyIconData.szTip, sizeof(m_notifyIconData.szTip), tooltip.c_str());
    
    // Create context menu
    if (!createContextMenu()) {
        std::cerr << "Failed to create context menu" << std::endl;
        return false;
    }
    
    return true;
}

bool SystemTray::show() {
    if (m_isVisible) return true;
    
    BOOL result = Shell_NotifyIcon(NIM_ADD, &m_notifyIconData);
    if (result) {
        m_isVisible = true;
        return true;
    }
    
    std::cerr << "Failed to add tray icon" << std::endl;
    return false;
}

bool SystemTray::hide() {
    if (!m_isVisible) return true;
    
    BOOL result = Shell_NotifyIcon(NIM_DELETE, &m_notifyIconData);
    if (result) {
        m_isVisible = false;
        return true;
    }
    
    return false;
}

bool SystemTray::updateTooltip(const std::string& tooltip) {
    if (!m_isVisible) return false;
    
    strcpy_s(m_notifyIconData.szTip, sizeof(m_notifyIconData.szTip), tooltip.c_str());
    return Shell_NotifyIcon(NIM_MODIFY, &m_notifyIconData) != FALSE;
}

bool SystemTray::processMessages() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

void SystemTray::setRefreshConnectionCallback(std::function<void()> callback) {
    m_onRefreshConnection = callback;
}

void SystemTray::setDisconnectCallback(std::function<void()> callback) {
    m_onDisconnect = callback;
}

void SystemTray::setExitCallback(std::function<void()> callback) {
    m_onExit = callback;
}

bool SystemTray::createWindow() {
    // Register window class
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    
    if (!RegisterClassW(&wc)) {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            std::cerr << "Failed to register window class: " << error << std::endl;
            return false;
        }
    }
    
    // Create hidden window
    m_hwnd = CreateWindowW(WINDOW_CLASS_NAME, L"FLRP Tray", 0, 0, 0, 0, 0, 
                           nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    
    return m_hwnd != nullptr;
}

bool SystemTray::createContextMenu() {
    m_contextMenu = CreatePopupMenu();
    if (!m_contextMenu) return false;
    
    // Add menu items
    AppendMenuW(m_contextMenu, MF_STRING, ID_REFRESH_CONNECTION, L"Refresh Discord Connection");
    AppendMenuW(m_contextMenu, MF_STRING, ID_DISCONNECT, L"Disconnect Rich Presence");
    AppendMenuW(m_contextMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(m_contextMenu, MF_STRING, ID_EXIT, L"Exit");
    
    return true;
}

void SystemTray::showContextMenu() {
    POINT cursor;
    GetCursorPos(&cursor);
    
    // Required for popup menu to work correctly
    SetForegroundWindow(m_hwnd);
    
    TrackPopupMenu(m_contextMenu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, m_hwnd, nullptr);
    
    // Required cleanup
    PostMessage(m_hwnd, WM_NULL, 0, 0);
}

void SystemTray::handleMenuCommand(UINT menuId) {
    switch (menuId) {
        case ID_REFRESH_CONNECTION:
            if (m_onRefreshConnection) {
                m_onRefreshConnection();
            }
            break;
            
        case ID_DISCONNECT:
            if (m_onDisconnect) {
                m_onDisconnect();
            }
            break;
            
        case ID_EXIT:
            if (m_onExit) {
                m_onExit();
            }
            break;
    }
}

LRESULT CALLBACK SystemTray::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_TRAY_ICON) {
        switch (lParam) {
            case WM_RBUTTONUP:
                if (g_trayInstance) {
                    g_trayInstance->showContextMenu();
                }
                break;
        }
        return 0;
    }
    else if (message == WM_COMMAND) {
        if (g_trayInstance) {
            g_trayInstance->handleMenuCommand(LOWORD(wParam));
        }
        return 0;
    }
    
    return DefWindowProc(hwnd, message, wParam, lParam);
}