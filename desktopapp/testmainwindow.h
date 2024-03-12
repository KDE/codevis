#ifndef TESTMAINWINDOW_H
#define TESTMAINWINDOW_H

#include <mainwindow.h>

#include <ct_lvtldr_nodestorage.h>
#include <ct_lvtmdl_debugmodel.h>
#include <ct_lvtqtc_undo_manager.h>

#include <QString>

namespace CodevisApplicationTesting {

class TestMainWindow : public MainWindow {
    Q_OBJECT
  public:
    explicit TestMainWindow(Codethink::lvtldr::NodeStorage& sharedNodeStorage,
                            Codethink::lvtqtc::UndoManager *undoManager = nullptr,
                            Codethink::lvtmdl::DebugModel *debugModel = nullptr);
    ~TestMainWindow() override = default;

    QString requestProjectName() override;
};

} // namespace CodevisApplicationTesting

#endif
