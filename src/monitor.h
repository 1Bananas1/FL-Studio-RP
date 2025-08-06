#pragma once
#include <windows.h>
#include <tlhelp32.h>

class ProcessMonitor {
    private:
        bool debugMode = false;
        
    public:
        void setDebugMode(bool debug);
        bool searchForFLStudio();
};