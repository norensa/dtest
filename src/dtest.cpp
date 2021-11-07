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

using namespace dtest;

static std::vector<std::string> dynamicTests;

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

int main(int argc, char *argv[]) {

    char cwd[PATH_MAX];
    getcwd(cwd, PATH_MAX);

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            findTests(argv[i]);
        }
    }
    else {
        findTests(cwd);
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
