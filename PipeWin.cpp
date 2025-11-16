//windows version of the problem
//10 processes
//searching prime numbers until it hits 10000

#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
using namespace std;

// Function to check if a number is prime
bool isPrime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return n == 2 || n == 3;
    for (int i = 5; (long long)i * i <= n; i += 6)
        if (n % i == 0 || n % (i + 2) == 0) return false;
    return true;
}

int main(int argc, char* argv[]) {
    const int MAX = 10000;
    const int WORKERS = 10;
    const int CHUNK = MAX / WORKERS;

    // If command-line arguments are passed, act as child process
    if (argc == 3) {
        int start = stoi(argv[1]);
        int end = stoi(argv[2]);
        for (int n = start; n <= end; ++n) {
            if (isPrime(n)) cout << n << "\n";
        }
        return 0;
    }

    // Parent process
    vector<PROCESS_INFORMATION> procs(WORKERS);
    vector<HANDLE> readHandles(WORKERS);

    for (int i = 0; i < WORKERS; ++i) {
        SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
        HANDLE outRd, outWr;
        if (!CreatePipe(&outRd, &outWr, &sa, 0)) {
            cerr << "Failed to create pipe\n";
            return 1;
        }
        SetHandleInformation(outRd, HANDLE_FLAG_INHERIT, 0);

        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.hStdOutput = outWr;
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        si.dwFlags |= STARTF_USESTDHANDLES;

        int start = i * CHUNK + 1;
        int end = (i + 1) * CHUNK;
        if (i == WORKERS - 1) end = MAX;

        // Create mutable command line string
        ostringstream oss;
        oss << "PipeWin.exe " << start << " " << end; 
        string cmd_str = oss.str();
        vector<char> cmd(cmd_str.begin(), cmd_str.end());
        cmd.push_back('\0'); // null-terminate

        PROCESS_INFORMATION pi;
        if (!CreateProcessA(NULL, cmd.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
            cerr << "CreateProcess failed: " << GetLastError() << "\n";
            return 1;
        }
        // parent closes write end
        CloseHandle(outWr); 
        readHandles[i] = outRd;
        procs[i] = pi;
    }

    // Read from children
    char buffer[256];
    for (int i = 0; i < WORKERS; ++i) {
        DWORD read;
        while (ReadFile(readHandles[i], buffer, sizeof(buffer) - 1, &read, NULL) && read > 0) {
            buffer[read] = '\0';
            cout << buffer;
        }
        CloseHandle(readHandles[i]);
        WaitForSingleObject(procs[i].hProcess, INFINITE);
        CloseHandle(procs[i].hProcess);
        CloseHandle(procs[i].hThread);
    }

    return 0;
}
