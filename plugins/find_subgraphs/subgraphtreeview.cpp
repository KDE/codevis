#include <QBoxLayout>
#include <QListWidgetItem>
#include <QPushButton>
#include <ct_lvtqtc_lakosentity.h>
#include <preferences.h>
#include <subgraphtreeview.h>

SubgraphListView::SubgraphListView(QWidget *parent)
{
    auto *mainLayout = new QBoxLayout(QBoxLayout::TopToBottom);
    d_view = new QListWidget(this);
    d_exportButton = new QPushButton(tr("Export"));
    mainLayout->addWidget(d_view);
    mainLayout->addWidget(d_exportButton);
    setLayout(mainLayout);
    connect(d_view, &QListWidget::itemClicked, this, &SubgraphListView::itemClicked);
}

void SubgraphListView::setGraphs(const std::vector<Graph>& graphs)
{
    std::cout << "Received a graph with " << graphs.size() << "Elements" << std::endl;

    QColor c = Preferences::self()->entityBackgroundColor();
    paintGraph(currSelectedIdx, c);

    d_graphs = graphs;
    d_view->clear();

    for (size_t i = 0; i < d_graphs.size(); i++) {
        auto *item = new QListWidgetItem();

        // Set the index of the graph, so we can refer to it later.
        item->setData(Qt::UserRole + 1, QVariant::fromValue(i));
        item->setText(tr("Graph %1").arg(i));
        d_view->addItem(item);
    }
}

void SubgraphListView::paintGraph(int idx, QColor color)
{
    if (idx == -1) {
        return;
    }

    Graph& graph = d_graphs[currSelectedIdx];
    for (const auto vd : boost::make_iterator_range(boost::vertices(graph))) {
        auto entity = graph[vd];
        entity.ptr->setColor(color);
    }
}

void SubgraphListView::itemClicked(QListWidgetItem *item)
{
    QColor c = Preferences::self()->entityBackgroundColor();
    paintGraph(currSelectedIdx, c);

    currSelectedIdx = item->data(Qt::UserRole + 1).toInt();
    paintGraph(currSelectedIdx, QColor(Qt::red));
}
