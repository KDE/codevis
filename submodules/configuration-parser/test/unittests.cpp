#include <fstream>
#include <string>
#include <vector>
#include <iostream>

#include "common/string_helpers.h"

#include "parser/meta_settings.h"
#include "parser/statemachine.h"

#include "qobject/dump_qobject.h"
#include "kconfig/dump_kconfig.h"
#include "qsettings/dump_qsettings.h"

#include <filesystem>

/* Check if the file represented by filename + one of the extensions exists on the filesystem */
bool check_file_exists(std::string filename, const std::vector<std::string>& extensions) {
    for(const auto& extension : extensions) {
        if(!std::filesystem::exists(filename + extension)) {
            std::cout << "Filename informed doesn't exist: " << (filename + extension) << std::endl;
            return false;
        }
    }
    return true;
}

/* Open the generated file and a existing, handmande file and verify them line-by-line,
 * the files should be exactly the same, if they are not then the generation
 * did something wrong and we need to fix that.
 * */
bool test_specific_file(const std::string& filename,
                        const std::pair<std::string, std::string>& extensions) {
    std::ifstream generated(filename + extensions.first);
    std::ifstream expected(filename + extensions.second);
    std::string gen, exp;
    while(!generated.eof() || !expected.eof()) {
        generated >> gen;
        expected >> exp;
        if (gen != exp) {
            std::cout << "Expected " << exp << " Generated " << gen << "\n";
            return false;
        }
    }
    return true;
}

/* generates the header and sources files and call them
 * test_specific_file for each of them.
 * */
int test_file(const std::string& filename) {
    std::cout << "Starting the test for:" << filename << "\n";
    std::ifstream file(filename + ".conf");

    MetaConfiguration conf = parse_configuration(file);

    // QObject
    if (check_file_exists(filename, {".qobject.h"})) {
        QObjectExport::dump_header(conf, filename + ".h", "");
        if (!test_specific_file(filename, {".h", ".qobject.h"})) {
            std::cout << "QObject error on" << filename << ".h" << "\n";
            return -1;
        }
    }

    if (check_file_exists(filename, {".qobject.cpp"})) {
        QObjectExport::dump_source(conf, filename + ".cpp");
        if (!test_specific_file(filename, {".cpp", ".qobject.cpp"})){
            std::cout << "QObject error on" << filename << ".cpp" << "\n";
            return -1;
        }
    }

    // KConfig
    if (check_file_exists(filename, {".kconfig.h"})) {
        KConfigExport::dump_header(conf, filename + ".h", "");
        if (!test_specific_file(filename, {".h", ".kconfig.h"})) {
            std::cout << "KConfig rror on" << filename << ".h" << "\n";
            return -1;
        }
    }

    if (check_file_exists(filename, {".kconfig.cpp"})) {
        KConfigExport::dump_source(conf, filename + ".cpp");
        if (!test_specific_file(filename, {".cpp", ".kconfig.cpp"})){
            std::cout << "KConfig error on" << filename << ".cpp" << "\n";
            return -1;
        }
    }

    // QSettings
    if (check_file_exists(filename, {".qsettings.h"})) {
        QSettingsExport::dump_header(conf, filename + ".h", "");
        if (!test_specific_file(filename, {".h", ".qsettings.h"})) {
            std::cout << "QSettings error on" << filename << ".h" << "\n";
            return -1;
        }
    }

    if (check_file_exists(filename, {".qsettings.cpp"})) {
        QSettingsExport::dump_source(conf, filename + ".cpp");
        if (!test_specific_file(filename, {".cpp", ".qsettings.cpp"})){
            std::cout << "QSettings error on" << filename << ".cpp" << "\n";
            return -1;
        }
    }

    std::cout << "Finished test for: " << filename << " without errors" << "\n";
    return 0;
}

/* Uses all the files passed thru parameters or, if no file
 * was avaliable via parameter, tries to find all the .conf
 * files in this folder and returns them.
 */
std::vector<std::string> find_filenames(int argc, char *argv[]) {
    std::vector<std::string> filenames;
    if (argc == 1) {
        auto files = adaptors::filter(directory_iterator(filesystem::absolute(".")),
                                    [](directory_entry& s){
            return algorithm::ends_with(s.path().generic_string(), ".conf");
        });
        for(const auto& file : files) {
            // extract the .conf
            std::string filename = file.path().generic_string();
            int substrSize = filename.find_last_of('.');
            filenames.push_back(filename.substr(0, substrSize));
        }
    } else for (int i = 1; i < argc; i++) {
        if (algorithm::ends_with(argv[i], ".conf")) {
            std::string file(argv[i]);
            filenames.push_back(file.substr(0, file.find_last_of('.')));
        }
    }
    return filenames;
}

/* runs all configuration files on the test cases*/
int main(int argc, char *argv[]) {
    std::cout << "Starting unittests" << "\n";
    std::vector<std::string> filenames = find_filenames(argc, argv);

    if (filenames.size() == 0) {
        std::cout << "Please fix the testcase." << "\n";
        return 0;
    }

    assert(filenames.size());
    std::cout << "Testing the following files:\n";
    for (const auto& file : filenames) {
        std::cout << '\t' << file << "\n";
    }

    for(const auto& file : filenames) {
        assert(test_file(file) == 0);
    }
    return 0;
}
