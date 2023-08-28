#include <iostream>
#include <cstdlib>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#pragma comment(lib, "Comctl32.lib")

#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <CommCtrl.h>
#include <vector>

std::wstring GetLastErrorAsString()
{
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0) {
        return L"";
    }

    LPWSTR messageBuffer = nullptr;
    size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, 0, (LPWSTR)&messageBuffer, 0, NULL);

    std::wstring message(messageBuffer, size);
    LocalFree(messageBuffer);

    return message;
}

std::wstring Utf8ToWide(const std::string& utf8Str)
{
    int requiredSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);

    std::vector<wchar_t> buffer(requiredSize);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, buffer.data(), requiredSize);

    return std::wstring(buffer.data());
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES; // Use the appropriate flags
    InitCommonControlsEx(&icex);

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc);

    HWND activeWnd = GetForegroundWindow();

    if (argc != 1)
    {
        MessageBoxW(activeWnd, L"Invalid amount of arguments", L"Error", MB_OK);
        return EXIT_FAILURE;
    }

    // Prompt deletion.
    if (MessageBoxW(activeWnd, L"Are you sure you want to delete this directory?", L"Delete Directory", MB_YESNO | MB_ICONQUESTION) != IDYES)
    {
        return EXIT_SUCCESS;
    }

    // Execute rmdir /S /Q on the provided directory with system
    const std::wstring directory = argv[0];

    // Create the command line
    std::wstring command = L"cmd.exe /C rmdir /S /Q \"" + directory + L"\"";

    // Use ShellExecuteW to execute the command
    SHELLEXECUTEINFOW executeInfo = { 0 };
    executeInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
    executeInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    executeInfo.lpFile = L"cmd.exe";
    executeInfo.lpParameters = command.c_str();
    executeInfo.nShow = SW_HIDE;  // Hide the command prompt window

    DWORD exitCode = 0;

    bool retry = false;

    do
    {
        retry = false;

        // Create the command line
        std::wstring command = L"cmd.exe /C chcp 65001 > nul && rmdir /S /Q \"" + directory + L"\"";

        // Use CreateProcessW to execute the command
        STARTUPINFOW si = { 0 };
        si.cb = sizeof(STARTUPINFO);
        PROCESS_INFORMATION pi = { 0 };

        HANDLE hChildStdErrRead = INVALID_HANDLE_VALUE;
        HANDLE hChildStdErrWrite = INVALID_HANDLE_VALUE;

        SECURITY_ATTRIBUTES saAttr{};
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        if (CreatePipe(&hChildStdErrRead, &hChildStdErrWrite, &saAttr, 0))
        {
            si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdOutput = hChildStdErrWrite;
            si.hStdError = hChildStdErrWrite;

            if (CreateProcessW(NULL, const_cast<LPWSTR>(command.c_str()), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
                CloseHandle(hChildStdErrWrite);
                WaitForSingleObject(pi.hProcess, INFINITE);

                DWORD exitCode;
                GetExitCodeProcess(pi.hProcess, &exitCode);

                // Close the process handles
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                CloseHandle(hChildStdErrWrite);

                if (exitCode != 0)
                {
                    char buffer[4096];
                    DWORD bytesRead;
                    std::string errorMessage;

                    while (ReadFile(hChildStdErrRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0)
                    {
                        errorMessage.append(buffer, bytesRead);
                    }

                    if (!errorMessage.empty())
                    {
                        if (MessageBoxW(activeWnd, Utf8ToWide(errorMessage).c_str(), L"Error", MB_RETRYCANCEL | MB_ICONERROR) == IDRETRY)
                        {
                            retry = true;
                        }
                    }
                    else
                    {
                        MessageBoxW(activeWnd, (L"Error code: " + GetLastErrorAsString()).c_str(), L"Error", MB_OK | MB_ICONERROR);
                    }
                }

                CloseHandle(hChildStdErrRead);
            }
            else
            {
                CloseHandle(hChildStdErrWrite);
                CloseHandle(hChildStdErrRead);
            }
        }
    } while (retry);

    return 0;
}
