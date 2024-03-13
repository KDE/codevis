#ifndef MERGE_PROJECT_DATABASES_H
#define MERGE_PROJECT_DATABASES_H

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include <result/result.hpp>

#include <codevis_project_helpers_export.h>

namespace Codethink {

class CODEVIS_PROJECT_HELPERS_EXPORT MergeProjects {
  public:
    enum class MergeProjectErrorEnum {
        NoError,
        Error,
        RemoveDatabaseError,
        DatabaseAccessError,
    };
    struct MergeProjectError {
        const MergeProjectErrorEnum errorVal;
        const std::string what;
    };

    using MergeDbProgressReportCallback = std::function<void(int idx, int database_size, const std::string& db_name)>;

    static cpp::result<void, MergeProjectError> mergeDatabases(std::vector<std::filesystem::path> project_or_databases,
                                                               std::filesystem::path resultingProject,
                                                               MergeDbProgressReportCallback progressReportCallback);
};

} // namespace Codethink
#endif
