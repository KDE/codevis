#include <QLoggingCategory>
#include <ct_lvtshr_debug_categories.h>

namespace {

const char *logCatAsStr(Codethink::lvtshr::LoggingCategory c)
{
    switch (c) {
    case Codethink::lvtshr::LoggingCategory::BackgroundGraphics:
        return "codevis.gui.BackgroundGraphics";
    case Codethink::lvtshr::LoggingCategory::Graphics:
        return "codevis.gui.Graphics";
    case Codethink::lvtshr::LoggingCategory::Interface:
        return "codevis.gui.Interface";
    case Codethink::lvtshr::LoggingCategory::Parsing:
        return "codevis.parsing.Parsing";
    case Codethink::lvtshr::LoggingCategory::TreeView:
        return "codevis.gui.TreeView";
    case Codethink::lvtshr::LoggingCategory::_End:
        break;
    }
    throw std::runtime_error("UNKNOWN LOGGING CATEGORY");
    return "unknown";
}
} // namespace

namespace Codethink::lvtshr {

const std::map<LoggingCategory, std::unique_ptr<QLoggingCategory>>& logCategories()
{
    static std::map<LoggingCategory, std::unique_ptr<QLoggingCategory>> c;
    if (c.empty()) {
        for (int i = 0; i < static_cast<int>(LoggingCategory::_End); ++i) {
            auto catEnum = static_cast<LoggingCategory>(i);
            c.insert({catEnum, std::make_unique<QLoggingCategory>(logCatAsStr(catEnum))});
        }
    }
    return c;
}

const QLoggingCategory& logCategory(LoggingCategory const& catEnum)
{
    return *logCategories().at(catEnum);
}

} // namespace Codethink::lvtshr
