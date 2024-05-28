#include <ct_lvtqtw_sqleditor.h>

#include <QAction>
#include <QBoxLayout>
#include <QGridLayout>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QMenu>
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

    tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(tableView, &QTableView::customContextMenuRequested, this, [this](const QPoint& pos) {
        auto *menu = new QMenu();
        auto *action = menu->addAction(tr("Load selected"));
        connect(action, &QAction::triggered, this, [this] {
            std::cout << "Running context menu thing";
            auto selectionModel = tableView->selectionModel();
            auto selection = selectionModel->selection();

            QSet<QString> qualifiedNames;
            for (const QModelIndex& item : selection.indexes()) {
                qualifiedNames.insert(item.data().toString());
                std::cout << item.data().toString().toStdString() << std::endl;
            }
            qDebug() << "Load requested" << qualifiedNames;
            Q_EMIT loadRequested(qualifiedNames);
        });
        menu->exec(mapToGlobal(pos));
    });
}
} // namespace Codethink::lvtqtw
