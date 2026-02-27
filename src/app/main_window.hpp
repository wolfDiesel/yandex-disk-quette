#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QSystemTrayIcon;
QT_END_NAMESPACE

namespace ydisquette {

struct CompositionRoot;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(CompositionRoot& root, QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void hideToTray();
    void showFromTray();

    CompositionRoot* root_;
    QSystemTrayIcon* tray_ = nullptr;
    bool trayEnabled_ = true;
};

}  // namespace ydisquette
