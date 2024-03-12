#include <QLoggingCategory>
#include <ct_lvtshr_debug_categories.h>

using namespace Codethink::lvtshr;

CategoryManager::CategoryManager() = default;

CategoryManager& CategoryManager::instance()
{
    static CategoryManager self;
    return self;
}

void CategoryManager::add(LoggingCategory cat, const QLoggingCategory *val)
{
    categories[cat] = val;
}
