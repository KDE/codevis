#ifndef CODEVIS_CT_LVTCLP_TOOL_H
#define CODEVIS_CT_LVTCLP_TOOL_H

#include <ct_lvtmdb_objectstore.h>
#include <filesystem>
#include <lvtclp_export.h>

namespace Codethink::lvtclp::fortran {

class LVTCLP_EXPORT Tool {
  public:
    Tool(std::vector<std::filesystem::path> const& files);
    Tool(std::filesystem::path const& compileCommandsJson);

    bool runPhysical(bool skipScan = false);
    bool runFull(bool skipPhysical = false);

    lvtmdb::ObjectStore& getObjectStore();

  private:
    std::vector<std::filesystem::path> files;
    lvtmdb::ObjectStore memDb;
};

} // namespace Codethink::lvtclp::fortran

#endif // CODEVIS_CT_LVTCLP_TOOL_H
