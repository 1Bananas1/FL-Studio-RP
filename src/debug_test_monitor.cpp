#include "monitor.h"
#include <iostream>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <algorithm>

void debugShowAllProcesses() {
    std::cout << "🔍 DEBUG: Showing ALL processes (looking for FL Studio):" << std::endl;
    
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create snapshot" << std::endl;
        return;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return;
    }
    
    int count = 0;
    do {
        // Convert wide character string to regular string
        std::string processName;
        
#ifdef UNICODE
        int size = WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, NULL, 0, NULL, NULL);
        if (size > 0) {
            processName.resize(size - 1);
            WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, &processName[0], size, NULL, NULL);
        }
#else
        processName = pe32.szExeFile;
#endif
        
        // Convert to lowercase for searching
        std::string lowerName = processName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        // Show ALL processes that might be FL Studio related
        if (lowerName.find("fl") != std::string::npos || 
            lowerName.find("studio") != std::string::npos ||
            lowerName.find("image") != std::string::npos ||  // Image-Line company
            lowerName.find("line") != std::string::npos) {
            
            std::cout << "  ⭐ POTENTIAL FL STUDIO: '" << processName << "' (lowercase: '" << lowerName << "')" << std::endl;
        }
        
        count++;
        
        // Show first few processes for reference
        if (count <= 10) {
            std::cout << "  Process " << count << ": '" << processName << "'" << std::endl;
        }
        
    } while (Process32Next(hProcessSnap, &pe32));
    
    CloseHandle(hProcessSnap);
    std::cout << "🔍 DEBUG: Total processes checked: " << count << std::endl << std::endl;
}
#endif

int main() {
    std::cout << "🧪 DEBUG Process Monitor Test" << std::endl;
    std::cout << "=============================" << std::endl;
    
#ifdef _WIN32
    // Show all F processes for debugging
    debugShowAllProcesses();
#endif
    
    ProcessMonitor monitor;
    
    std::cout << "Now testing FL Studio detection:" << std::endl;
    
    // Test the monitor
    for (int i = 0; i < 5; i++) {
        std::cout << "🔍 Test " << (i + 1) << "/5: ";
        
        if (monitor.searchForFLStudio()) {
            std::cout << "✅ FL Studio IS running!" << std::endl;
        } else {
            std::cout << "❌ FL Studio NOT found" << std::endl;
        }
        
        // Wait 2 seconds before next check
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    std::cout << std::endl;
    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}