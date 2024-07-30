#ifndef CT_LVTCLP_CPP_TOOL_CONSTANTS
#define CT_LVTCLP_CPP_TOOL_CONSTANTS

#include <filesystem>
#include <llvm/Support/GlobPattern.h>
#include <string>
#include <vector>

/* Some elements that we need in every single instance
 * of the CPP Tools, for every thread, created millions of times
 * during the runtime of the program.
 */
struct CppToolConstants {
    const std::filesystem::path prefix;
    const std::filesystem::path buildPath;
    const std::filesystem::path databasePath;
    const std::vector<std::filesystem::path> nonLakosianDirs;
    const std::vector<std::pair<std::string, std::string>> thirdPartyDirs;
    const std::vector<llvm::GlobPattern> ignoreGlobs;
    const std::vector<std::string> userProvidedExtraCompileCommandsArgs;
    const unsigned numThreads = 1;
    const bool enableLakosianRules = false;
    const bool printToConsole = false;
};

#endif
