#include <ct_lvtshr_debug_categories.h>

using namespace Codethink::lvtshr;

CategoryManager::CategoryManager() = default;

CategoryManager& CategoryManager::instance()
{
    static CategoryManager self;
    return self;
}

void CategoryManager::add(const QLoggingCategory *val)
{
    categories.append(val);
}
QList<const QLoggingCategory *> CategoryManager::getCategories()
{
    return categories;
}