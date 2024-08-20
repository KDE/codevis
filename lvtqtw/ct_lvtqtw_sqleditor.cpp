#include <ct_lvtqtw_sqleditor.h>

#include <KMessageWidget>
#include <QAction>
#include <QBoxLayout>
#include <QGridLayout>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
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

    connect(btnRun, &QPushButton::clicked, this, [doc, this] {
        model->setQuery(doc->text());
    });

    tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    auto *msg = new KMessageWidget();
    msg->setVisible(false);

    connect(model, &lvtmdl::SqlModel::invalidQueryTriggered, this, [msg](const QString& err, const QString& query) {
        msg->setText(err);
        msg->setVisible(true);
        std::ignore = query;
    });

    connect(tableView, &QTableView::customContextMenuRequested, this, [this](const QPoint& pos) {
        auto *menu = new QMenu();
        auto *action = menu->addAction(tr("Load selected"));
        connect(action, &QAction::triggered, this, [this] {
            auto selectionModel = tableView->selectionModel();
            auto selection = selectionModel->selection();

            QSet<QString> qualifiedNames;
            for (const QModelIndex& item : selection.indexes()) {
                qualifiedNames.insert(item.data().toString());
                std::cout << item.data().toString().toStdString() << std::endl;
            }
            Q_EMIT loadRequested(qualifiedNames);
        });
        menu->exec(mapToGlobal(pos));
    });

    auto *scrollArea = new QScrollArea(this);
    auto *umlLabel = new QLabel();
    scrollArea->setWidget(umlLabel);

    auto schema = QPixmap(":/icons/db_schema_svg");
    umlLabel->setGeometry(0, 0, schema.width(), schema.height());
    umlLabel->setPixmap(schema);

    auto splitter = new QSplitter(Qt::Orientation::Horizontal, this);

    auto *leftWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tableView);
    layout->addWidget(msg);
    layout->addWidget(view);
    layout->addWidget(btnRun);
    leftWidget->setLayout(layout);

    splitter->addWidget(leftWidget);
    splitter->addWidget(scrollArea);

    auto *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(splitter);
    setLayout(mainLayout);
}
} // namespace Codethink::lvtqtw

#include "moc_ct_lvtqtw_sqleditor.cpp"
