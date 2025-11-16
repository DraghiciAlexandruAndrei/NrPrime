#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;

// Check if a number is prime
bool isPrime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return n == 2 || n == 3;
    for (int i = 5; (long long)i * i <= n; i += 6)
        if (n % i == 0 || n % (i + 2) == 0) return false;
    return true;
}

int main() {
    const int MAX = 10000;
    const int WORKERS = 10;
    const int CHUNK = MAX / WORKERS;

    vector<int> pipes_fd[WORKERS]; // parent reads from child

    vector<pid_t> pids(WORKERS);

    for (int i = 0; i < WORKERS; ++i) {
        int p[2];
        if (pipe(p) == -1) { perror("pipe"); exit(1); }
        int read_fd = p[0];
        int write_fd = p[1];

        pid_t pid = fork();
        if (pid == -1) { perror("fork"); exit(1); }

        if (pid == 0) {
            // Child process
            close(read_fd); // close read end

            int start = i * CHUNK + 1;
            int end = (i + 1) * CHUNK;
            if (i == WORKERS - 1) end = MAX;

            // Compute primes and write to pipe
            for (int n = start; n <= end; ++n) {
                if (isPrime(n)) {
                    string s = to_string(n) + "\n";
                    write(write_fd, s.c_str(), s.size());
                }
            }

            close(write_fd); // signal EOF
            exit(0);
        }
        else {
            // Parent process
            close(write_fd); // close write end
            pipes_fd[i].push_back(read_fd);
            pids[i] = pid;
        }
    }

    // Parent reads from all children
    for (int i = 0; i < WORKERS; ++i) {
        FILE* f = fdopen(pipes_fd[i][0], "r");
        if (!f) continue;
        char* line = nullptr;
        size_t len = 0;
        while (getline(&line, &len, f) != -1) {
            printf("%s", line);
        }
        free(line);
        fclose(f);
    }

    // Wait for all children
    for (int i = 0; i < WORKERS; ++i) waitpid(pids[i], nullptr, 0);

    return 0;
}
