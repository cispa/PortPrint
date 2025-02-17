#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <cpuid.h>
#include <errno.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include "victims.h"
#include "compression.h"
#include "contention_primitives.h"

#define countof(x) (sizeof(x)/sizeof(*(x)))

#define RESULT_DIR          "measurements"
#define NUM_RUNS            0x800
#define NUM_SAMPLES         600

const struct contention_arch * arch = &alder_lake_p;
static uint32_t cores = 0;

static int create_directory_recursive(const char *path) {
    char parent[512];
    char *slash;

    if (mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0 || errno == EEXIST) {
        return 0;
    }

    if (errno == ENOENT) {
        strncpy(parent, path, sizeof(parent));
        slash = strrchr(parent, '/');
        if (slash) {
            *slash = '\0';
            if (create_directory_recursive(parent) == 0) {
                return mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            }
        }
    }

    perror("mkdir");
    return -1;
}

static int remove_files_in_directory(const char *path) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    char filepath[4096];

    if (!dir) {
        perror("opendir");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_REG) { // Regular file
            if (unlink(filepath) != 0) {
                perror("unlink");
                closedir(dir);
                return -1;
            }
        }
    }

    closedir(dir);
    return 0;
}

static void set_processor_affinity(unsigned int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_t current_thread = pthread_self();
    pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

static uint32_t get_sibling_cores(){
    const static size_t max_cpu_cores = 0x100;
    char *line = NULL;
    unsigned int corenum;
    int parsed, count = 0, i, j;
    size_t len = 0;
    FILE* info;
    uint8_t cpu_cores[max_cpu_cores];

    info = fopen( "/proc/cpuinfo", "r");
    if(!info)
        return 1 << 16;

    // Get amount of executable memory regions
    while (getline(&line, &len, info) > 0 && count < max_cpu_cores) {
        parsed = sscanf(line, "core id\t\t: %u", &corenum);
        free(line);
        line = NULL;
        if(parsed <= 0)
            continue;
        cpu_cores[count++] = (uint8_t) corenum;
    }

    fclose(info);

    for(i = count - 1; i >= 0; i--) {
        for(j = i - 1; j >= 0; j--) {
            if(cpu_cores[i] == cpu_cores[j])
                return (((uint32_t) i) << 16) | (uint32_t) j;
        }
    }

    return 1 << 16;
}

static void attacker(const char* out_dir) {
    const unsigned int num_ports = arch->num_ports;
    unsigned char failed;
    unsigned i, j, p, q;
    static uint64_t result_buffer[NUM_SAMPLES], sum_buffer[countof(result_buffer)] = {0, }, timing;
    char namebuf[512];
    ZstdFile* files[num_ports];

    memset(result_buffer, 0, sizeof(result_buffer));

    // Open output files
    for (i = 0; i < countof(files); i++) {
        snprintf(namebuf, sizeof(namebuf), "%s/%s-%s-%dsumsof1.bin", out_dir, arch->name, arch->port_names[i], NUM_RUNS);
        if (access(namebuf, F_OK) >= 0)
            remove(namebuf);

        files[i] = zstdOpen(namebuf, 19);
    }

    set_processor_affinity(cores & 0xff);

    // Make sure everything is cached on the victim's side
    for (i = 0; i < 5; i++) {
        *sync_page = 1;
        while(*sync_page) {asm volatile("lfence");}
    }

    // Run the actual attack
    for(i = 0; i < countof(files); i++) {  // For each port
        for(q = 0; q < NUM_RUNS; q++) {
            // Yield now so that we (hopefully) don't get interrupted during profiling
            sched_yield();

            do { // One sampling run
                failed = 0;
                memset(sum_buffer, 0, sizeof(sum_buffer));

                // Tell the victim to start now
                *sync_page = 1;

                 // Record timings for port
                arch->contenders[i](result_buffer, countof(result_buffer));

                // Convert output of contention primitive into usable format, and check for error conditions
                for (p = 0; p < countof(sum_buffer) - 1; p++) {
                    timing = (result_buffer[p] & ((1ul << 32) - 1)) - (result_buffer[p] >> 32);
                    if (timing > UINT16_MAX) {
                        failed = 1;
                        break;
                    }
                    sum_buffer[p] += timing;
                }

                // Wait for victim to complete whatever it is doing
                do {asm volatile("lfence");} while(*sync_page);
            } while(failed);

            // Save output
            zstdWrite(files[i], sum_buffer, sizeof(sum_buffer));
        }
        zstdClose(files[i]);
    }
}

int main(int argc, char* argv[]) {
    unsigned int i;
    pid_t victim;
    char namebuf[512];

    if (argc < 2) {
        printf("Usage: %s <achitecture>\nPossible values for architecture: ", argv[0]);
        for (i = 0; i < countof(architectures); i++)
            printf("%s ", architectures[i]->name);
        printf("\n");
        return 0;
    }

    for (i = 0; i < countof(architectures); i++) {
        if (strcmp(argv[1], architectures[i]->name) == 0) {
            arch = architectures[i];
            break;
        }
    }
    if (i >= countof(architectures)) {
        printf("Unknown architecture '%s'\n", argv[1]);
        return EXIT_FAILURE;
    }

    cores = get_sibling_cores();

    // Set up sync page for communication between victim and attacker processes
    sync_page = mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    *sync_page = 0;

    printf("Port contention experiment for %s. Running on cores %u and %u!\n", arch->name, cores & 0xff, cores >> 16);

    for (i = 0; i < countof(victims); i++) {
        if (!is_extension_supported(victims[i]->extensions))
            continue;

        snprintf(namebuf, sizeof(namebuf), RESULT_DIR "/%s/%s-%s", arch->name, victims[i]->name, source_project_name(victims[i]->source));
        create_directory_recursive(namebuf);
        remove_files_in_directory(namebuf);
        printf("%s-%s\n", victims[i]->name, source_project_name(victims[i]->source));

        // Fork and pivot to SMT sibling cores
        victim = fork();
        if (!victim) {
            prctl(PR_SET_PDEATHSIG, SIGHUP);
            set_processor_affinity(cores >> 16);
            // Run victim - we do not return
            victims[i]->run();
        }

        set_processor_affinity(cores & 0xff);
        // Run attacker, and kill victim when done
        attacker(namebuf);
        kill(victim, SIGKILL);
    }

    return 0;
}
