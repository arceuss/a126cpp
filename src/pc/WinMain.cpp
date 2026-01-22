#include <windows.h>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>

// Forward declaration of the actual main function
int minecraft_main(int argc, char *argv[]);

// Windows entry point that calls our C++ main function
int CALLBACK WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    // Simple argc/argv setup for SDL
    int argc = 1;
    char** argv = new char*[1];
    argv[0] = new char[256];
    strcpy_s(argv[0], 256, "Alpha126Cpp");
    
    try
    {
        // Debug: Show that we're starting
        OutputDebugStringA("Alpha126Cpp.exe: Starting minecraft_main\n");
        
        // Call the real main function
        int result = minecraft_main(argc, argv);
        
        OutputDebugStringA("Alpha126Cpp.exe: minecraft_main returned\n");
        
        // Clean up
        delete[] argv[0];
        delete[] argv;
        
        return result;
    }
    catch (const std::exception &e)
    {
        std::string msg = std::string("Alpha126Cpp.exe: Exception: ") + e.what();
        OutputDebugStringA(msg.c_str());
        MessageBoxA(nullptr, e.what(), "Fatal Error", MB_OK | MB_ICONERROR);
        
        delete[] argv[0];
        delete[] argv;
        
        return 1;
    }
    catch (...)
    {
        OutputDebugStringA("Alpha126Cpp.exe: Unknown exception\n");
        MessageBoxA(nullptr, "Unknown error occurred", "Fatal Error", MB_OK | MB_ICONERROR);
        
        delete[] argv[0];
        delete[] argv;
        
        return 1;
    }
}
