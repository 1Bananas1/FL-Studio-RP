#include "monitor.h"
#include <iostream>
#include <algorithm>
#include <string>

#ifdef _WIN32

void ProcessMonitor::setDebugMode(bool debug) {
    debugMode = debug;
}

bool ProcessMonitor::searchForFLStudio() {
    // Create snapshot of all processes
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "❌ CreateToolhelp32Snapshot failed" << std::endl;
        return false;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32); // Must set dwSize before calling Process32First
    
    // Get the first process
    if (!Process32First(hProcessSnap, &pe32)) {
        std::cerr << "❌ Process32First failed" << std::endl;
        CloseHandle(hProcessSnap);
        return false;
    }
    
    // Walk through processes until we find FL Studio
    do {
        // Convert wide character string to regular string
        std::string processName;
        
#ifdef UNICODE
        // Convert from WCHAR to std::string
        int size = WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, NULL, 0, NULL, NULL);
        if (size > 0) {
            processName.resize(size - 1);
            WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, &processName[0], size, NULL, NULL);
        }
#else
        // For non-Unicode builds (rare)
        processName = pe32.szExeFile;
#endif
        
        // Convert to lowercase for comparison
        std::transform(processName.begin(), processName.end(), processName.begin(), ::tolower);
        
        // Check for FL Studio processes (removed early exit optimization)
        if (processName == "fl64.exe" || 
            processName == "fl64" ||        // Sometimes shows without .exe
            processName == "fl32.exe" || 
            processName == "fl32" ||
            processName == "fl.exe" ||
            processName == "fl" ||
            processName == "flstudio.exe" ||
            processName == "flstudio" ||
            (processName.find("fl") == 0 && processName.find("studio") != std::string::npos)) {
            
            if (debugMode) {
                std::cout << "🎵 Found FL Studio process: " << processName << std::endl;
            }
            CloseHandle(hProcessSnap);
            return true;
        }
        
    } while (Process32Next(hProcessSnap, &pe32));
    
    CloseHandle(hProcessSnap);
    return false;
}

#else
// Non-Windows implementation (placeholder)
bool ProcessMonitor::searchForFLStudio() {
    std::cerr << "❌ Process monitoring not implemented for this platform" << std::endl;
    return false;
}
#endif