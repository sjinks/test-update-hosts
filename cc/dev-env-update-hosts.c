#if defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif // defined(__linux__) || defined(__APPLE__)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

static int update_hosts(const char* fname, const char** domain, size_t ndomains)
{
    FILE* hosts = fopen(fname, "r+");
    if (!hosts) {
        perror("Error opening hosts file");
        return EXIT_FAILURE;
    }

    if (-1 == fseek(hosts, 0, SEEK_END)) {
        perror("Error seeking to end of hosts file");
        fclose(hosts);
        return EXIT_FAILURE;
    }

    int status = EXIT_SUCCESS;
    for (size_t i = 0; i < ndomains; ++i) {
        if (fprintf(hosts, "127.0.0.1\t%s\n", domain[i]) < 0) {
            perror("Error writing to hosts file");
            status = EXIT_FAILURE;;
            break;
        }
    }

    if (0 != fclose(hosts)) {
        perror("Error closing hosts file");
        status = EXIT_FAILURE;
    }

    return status;
}

static char* my_strdup(const char* s)
{
    if (s == NULL) {
        return NULL;
    }

    size_t len = strlen(s) + 1;
    char* p = malloc(len);
    if (p) {
        memcpy(p, s, len);
    }

    return p;
}

#if defined __linux__ || defined __APPLE__

static char* find_executable(const char* name)
{
    char* path_env = getenv("PATH");
    char* path     = my_strdup(path_env ? path_env : "/usr/bin");
    if (!path) {
        perror("Error allocating memory");
        return NULL;
    }

    size_t len  = strlen(path) + strlen(name) + 2;
    char* fname = malloc(len);
    if (!fname) {
        free(path);
        perror("Error allocating memory");
        return NULL;
    }

    char* dir = strtok(path, ":");
    while (dir) {
        snprintf(fname, len, "%s/%s", dir, name);
        if (access(fname, X_OK) == 0) {
            free(path);
            return fname;
        }

        dir = strtok(NULL, ":");
    }

    free(path);
    free(fname);
    return NULL;
}

#endif // defined(__linux__) || defined(__APPLE__)

static bool is_valid_label(const char* label)
{
    size_t len = strlen(label);
    if (len == 0 || len > 63 || label[0] == '-' || label[len - 1] == '-') {
        return false;
    }

    for (size_t i = 0; i < len; ++i) {
        if (!isalnum(label[i]) && label[i] != '-') {
            return false;
        }
    }

    return true;
}

static bool is_valid_domain(const char* s)
{
    if (s == NULL || strlen(s) > 255) {
        return false;
    }

    char* domain  = my_strdup(s);
    if (!domain) {
        return false;
    }

    bool valid = true;
    const char* label = strtok(domain, ".");
    while (label) {
        if (!is_valid_label(label)) {
            valid = false;
            break;
        }

        label = strtok(NULL, ".");
    }

    free(domain);
    return valid;
}

static char* get_hosts_file_path()
{
#if defined(_WIN32)
    char* system_root = getenv("SystemRoot");
    if (!system_root || !*system_root) {
        perror("Error getting SystemRoot environment variable");
        return NULL;
    }

    const size_t max_path_length = 1024;
    char* hosts_path = (char*)malloc(max_path_length * sizeof(char));
    if (hosts_path == NULL) {
        perror("Memory allocation error");
        return NULL;
    }

    int n = snprintf(hosts_path, max_path_length, "%s\\System32\\drivers\\etc\\hosts", system_root);
    if (n < 0 || n >= (int) max_path_length) {
        perror("Error constructing hosts file path");
        return NULL;
    }

    return hosts_path;
#else
    return my_strdup("/etc/hosts");
#endif
}

static int escalate_privilege(int argc, char** argv) {
#if defined(__linux__) || defined(__APPLE__)
    if (geteuid() != 0) {
        char* elevator = find_executable("pkexec") ?: find_executable("sudo");
        if (!elevator) {
            fputs("Error: No suitable privilege escalation tool found\n", stderr);
            return EXIT_FAILURE;
        }

        char* new_argv[argc + 2];
        new_argv[0] = elevator;
        for (int i = 0; i < argc; ++i) {
            new_argv[i + 1] = argv[i];
        }
        new_argv[argc + 1] = NULL;

        execvp(elevator, new_argv);
        perror("Error executing privilege escalation tool");
        free(elevator);
        return EXIT_FAILURE;
    }
#endif
    return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
    if (escalate_privilege(argc, argv) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    const char* domains[argc - 1];
    size_t ndomains = 0;
    for (int i = 1; i < argc; ++i) {
        if (is_valid_domain(argv[i])) {
            domains[ndomains++] = argv[i];
        }
    }

    if (ndomains == 0) {
        fputs("Error: No valid domains provided\n", stderr);
        return EXIT_FAILURE;
    }

    char* hosts = get_hosts_file_path();
    if (!hosts) {
        fputs("Error: Unable to determine hosts file path\n", stderr);
        return EXIT_FAILURE;
    }

    int code = update_hosts(hosts, domains, ndomains);
    free(hosts);
    return code;
}
