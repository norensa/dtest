/*
 * Copyright (c) 2021 Noah Orensa.
 * Licensed under the MIT license. See LICENSE file in the project root for details.
*/

#include <dtest_core/test.h>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <libgen.h>
#include <linux/limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <dtest_core/util.h>
#include <vector>
#include <string>
#include <unordered_set>

using namespace dtest;

static std::vector<std::string> dynamicTests;

static bool runWorker = false;
static uint32_t workerId = 0;
static std::unordered_set<std::string> modules;

static void loadTests(const char *path) {
    std::cerr << "Loading " << path << "\n";

    dynamicTests.push_back(path);

    void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL) {
        std::cerr << dlerror() << std::endl;
        exit(1);
    }
    Memory::reinitialize(handle);
}

static void findTests(const char *path) {
    struct stat st;
    stat(path, &st);

    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        struct dirent *dir;
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (
                    strcmp(dir->d_name, ".") != 0
                    && strcmp(dir->d_name, "..") != 0
                    && strcmp(dir->d_name, "dtest") != 0
                ) {
                    char *child = (char *) malloc(PATH_MAX);
                    strcpy(child, path);
                    strcat(child, "/");
                    strcat(child, dir->d_name);
                    findTests(child);
                    free(child);
                }
            }
            closedir(d);
        }
    }
    else {
        size_t len = strlen(path);
        if (len >= 9 && strcasecmp(path + len - 9, ".dtest.so") == 0) {
            loadTests(path);
        }
    }
}

void printHelp() {
    std::cout <<
        "Usage: dtest <options> <test directories or files>\n"
        "\n"
        "Options\n"
        "    --port <port-num>          Specifies the port number for the test driver.\n"
        "    --workers <num-workers>    Specifies the number of remote workers available.\n"
        "    --driver <address>         Connects to a remote test driver at <address>.\n"
        "    --worker-id <id>           Runs a test worker, using <id> as its unique\n"
        "                               identifier.\n"
        "    --module <test-module>     Runs one or more test modules and skips all other\n"
        "                               tests.\n"
        "\n\n"
    ;
}

void parseArguments(int argc, char *argv[], const char *cwd) {
    bool gotTestLocation = false;

    for (int i = 0; i < argc; ++i) {
        if (argv[i][0] == '-' || strncmp(argv[i], "--", 2) == 0) {
            if (strcasecmp(argv[i], "--port") == 0) {
                DriverContext::instance->setPort(atoi(argv[++i]));
            }
            else if (strcasecmp(argv[i], "--workers") == 0) {
                uint32_t numWorkers = atoi(argv[++i]);
                for (uint32_t i = 0; i < numWorkers; ++i) {
                    DriverContext::instance->addWorker(i);
                }
            }
            else if (strcasecmp(argv[i], "--driver") == 0) {
                runWorker = true;
                DriverContext::instance->setAddress(argv[++i]);
            }
            else if (strcasecmp(argv[i], "--worker-id") == 0) {
                workerId = atoi(argv[++i]);
            }
            else if (strcasecmp(argv[i], "--module") == 0) {
                modules.insert(argv[++i]);
            }
            else if (strcasecmp(argv[i], "-h") == 0 || strcasecmp(argv[i], "--help") == 0) {
                printHelp();
                exit(0);
            }
            else {
                std::cerr << "Unknown option '" << argv[i] << "'\n\n";
                exit(1);
            }
        }
        else if (access(argv[i], F_OK) == 0) {
            findTests(argv[i]);
            gotTestLocation = true;
        }
        else {
            std::cerr << "Argument '" << argv[i] << "' is not a valid option or test location.\n\n";
            exit(1);
        }
    }

    if (! gotTestLocation) {
        findTests(cwd);
    }
}

int main(int argc, char *argv[]) {
    char cwd[PATH_MAX];
    getcwd(cwd, PATH_MAX);

    parseArguments(argc - 1, argv + 1, cwd);

    if (runWorker) {
        try {
            Test::runWorker(workerId);
            exit(0);
        }
        catch (...) {
            exit(1);
        }
    }

    Test::logStatsToStderr(true);

    std::fstream logFile;
    logFile.open("dtest.log.json", std::ios_base::out | std::ios_base::trunc);

    bool success = Test::runAll(
        {
            { "executable", argv[0] },
            { "args", jsonify(argc - 1, argv + 1, 2) },
            { "working_dir", cwd },
            { "loaded_test_files", jsonify(dynamicTests, 2) },
        },
        modules,
        logFile
    );
    logFile.close();

    if (success) {
        std::cerr << "\nAll tests OK. See dtest.log.json for more details.\n\n";
        exit(0);
    }
    else {
        std::cerr << "\nOne or more tests failed. See dtest.log.json for more details.\n\n";
        exit(1);
    }
}
