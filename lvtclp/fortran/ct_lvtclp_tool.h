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
    void setSharedMemDb(std::shared_ptr<lvtmdb::ObjectStore> const& sharedMemDb);

  private:
    std::vector<std::filesystem::path> files;

    // The tool can be used either with a local memory database or with a
    // shared one. Only one can be used at a time. The default is to use
    // localMemDb. If setMemDb(other) is called, will ignore the local one.
    lvtmdb::ObjectStore localMemDb;
    std::shared_ptr<lvtmdb::ObjectStore> sharedMemDb = nullptr;
    [[nodiscard]] inline lvtmdb::ObjectStore& memDb()
    {
        return this->sharedMemDb ? *this->sharedMemDb : this->localMemDb;
    }
};

} // namespace Codethink::lvtclp::fortran

#endif // CODEVIS_CT_LVTCLP_TOOL_H
