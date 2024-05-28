#include <ct_lvtqtw_sqleditor.h>

#include <QBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QSplitter>
#include <QTableView>

namespace Codethink::lvtqtw {
SqlEditor::SqlEditor(lvtldr::NodeStorage& sharedStorage, QWidget *parent): QWidget(parent)
{
    auto *editor = KTextEditor::Editor::instance();
    auto *doc = editor->createDocument(this);
    auto *view = doc->createView(this);

    doc->setHighlightingMode("SQL");

    tableView = new QTableView(this);
    model = new lvtmdl::SqlModel(sharedStorage);
    tableView->setModel(model);

    auto btnRun = new QPushButton(tr("Run Query"));
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tableView);
    layout->addWidget(view);
    layout->addWidget(btnRun);
    setLayout(layout);

    connect(btnRun, &QPushButton::clicked, this, [doc, this] {
        model->setQuery(doc->text());
    });
}
} // namespace Codethink::lvtqtw
