#pragma once

#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QWidget>

#include <graphdefinition.h>

class SubgraphListView : public QWidget {
    Q_OBJECT

  public:
    SubgraphListView(QWidget *parent);
    void setGraphs(const std::vector<Graph>& graphs);
    void itemClicked(QListWidgetItem *item);

  private:
    int currSelectedIdx = -1;
    QListWidget *d_view;
    QPushButton *d_exportButton;
    std::vector<Graph> d_graphs;
};
