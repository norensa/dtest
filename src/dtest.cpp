#include <test.h>

#include <fstream>

using namespace dtest;

int main(int argc, char *argv[]) {
    Test::logStatsToStderr(true);

    std::fstream logFile;
    logFile.open("test.log", std::ios_base::out | std::ios_base::trunc);

    bool success = Test::runAll(logFile);

    logFile.close();
    std::cerr << "\nSee test.log for more details.\n\n";

    if (success) exit(0);
    else exit(1);
}
