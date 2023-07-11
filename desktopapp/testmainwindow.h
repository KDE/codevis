#ifndef TESTMAINWINDOW_H
#define TESTMAINWINDOW_H

#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtmdl_debugmodel.h>
#include <ct_lvtqtc_undo_manager.h>
#include <mainwindow.h>

class TestMainWindow : public MainWindow {
    Q_OBJECT
  public:
    explicit TestMainWindow(Codethink::lvtldr::NodeStorage& sharedNodeStorage,
                            Codethink::lvtqtc::UndoManager *undoManager = nullptr,
                            Codethink::lvtmdl::DebugModel *debugModel = nullptr);

    QString requestProjectName() override
    {
        return QStringLiteral("__test_project__");
    }
};

#endif
