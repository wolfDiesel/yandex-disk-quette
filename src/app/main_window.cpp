#include "main_window.hpp"
#include "composition_root.hpp"
#include "settings/domain/app_settings.hpp"
#include <QApplication>
#include <QCloseEvent>
#include <QEvent>
#include <QMenu>
#include <QStyle>
#include <QSystemTrayIcon>

namespace ydisquette {

MainWindow::MainWindow(CompositionRoot& root, QWidget* parent)
    : QMainWindow(parent), root_(&root) {
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        tray_ = new QSystemTrayIcon(this);
        QIcon icon = qApp->windowIcon();
        if (icon.isNull())
            icon = style()->standardIcon(QStyle::SP_ComputerIcon);
        tray_->setIcon(icon);
        tray_->setToolTip(QStringLiteral("Y.Disquette"));
        QMenu* menu = new QMenu(this);
        QAction* showAction = menu->addAction(tr("Show"));
        connect(showAction, &QAction::triggered, this, &MainWindow::showFromTray);
        menu->addSeparator();
        QAction* quitAction = menu->addAction(tr("Quit"));
        connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
        tray_->setContextMenu(menu);
        connect(tray_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger)
                showFromTray();
        });
        tray_->show();
        trayEnabled_ = true;
    } else {
        trayEnabled_ = false;
    }
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* event) {
    if (trayEnabled_ && tray_ && tray_->isVisible()) {
        auto s = root_->getSettingsUseCase().run();
        if (s.closeToTray) {
            event->ignore();
            hideToTray();
            return;
        }
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange && trayEnabled_ && tray_ && tray_->isVisible()) {
        auto s = root_->getSettingsUseCase().run();
        if (s.hideToTray && isMinimized()) {
            hideToTray();
            event->accept();
            return;
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::hideToTray() {
    hide();
}

void MainWindow::showFromTray() {
    showNormal();
    show();
    raise();
    activateWindow();
}

}  // namespace ydisquette
