#include <test.h>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <libgen.h>
#include <linux/limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dlfcn.h>

using namespace dtest;

static void loadTests(const char *path) {
    std::cerr << "Loading " << path << "\n";

    void *handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (handle == NULL) {
        std::cerr << dlerror() << std::endl;
        exit(1);
    }
}

static void findTests(const char *path, bool load = false) {
    struct stat st;
    stat(path, &st);

    if (S_ISDIR(st.st_mode)) {

        if (! load) {
            char *p = strdup(path);
            if (strcasecmp(basename(p), "test") == 0) {
                findTests(path, true);
                free(p);
                return;
            }
            free(p);
        }

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
                    findTests(child, load);
                    free(child);
                }
            }
            closedir(d);
        }
    }
    else if (load) {
        auto ext = strrchr(path, '.');
        if (ext != NULL && strcasecmp(ext, ".so") == 0) loadTests(path);
    }
}

int main(int argc, char *argv[]) {

    if (argc > 1) {
        for (int i = 1; i < argc; ++i) {
            findTests(argv[i]);
        }
    }
    else {
        char cwd[PATH_MAX];
        getcwd(cwd, PATH_MAX);
        findTests(cwd);
    }
    Memory::reinitialize();

    Test::logStatsToStderr(true);

    std::fstream logFile;
    logFile.open("dtest.log.json", std::ios_base::out | std::ios_base::trunc);

    bool success = Test::runAll(logFile);
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
