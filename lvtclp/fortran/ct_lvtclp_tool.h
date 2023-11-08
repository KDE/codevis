#ifndef CODEVIS_CT_LVTCLP_TOOL_H
#define CODEVIS_CT_LVTCLP_TOOL_H

#include <lvtclp_export.h>

namespace Codethink::lvtclp::fortran {

class LVTCLP_EXPORT Tool {
  public:
    bool runPhysical(bool skipScan = false);
    bool runFull(bool skipPhysical = false);
};

} // namespace Codethink::lvtclp::fortran

#endif // CODEVIS_CT_LVTCLP_TOOL_H
