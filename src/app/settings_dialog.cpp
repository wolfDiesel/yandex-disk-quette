#include "settings_dialog.hpp"
#include "composition_root.hpp"
#include "json_config.hpp"
#include "settings/domain/app_settings.hpp"
#include "ui_settings_dialog.h"
#include <QFileDialog>

namespace ydisquette {

SettingsDialog::SettingsDialog(CompositionRoot& root, QWidget* parent)
    : QDialog(parent), root_(&root), ui_(new Ui::SettingsDialog) {
    ui_->setupUi(this);
    ydisquette::settings::AppSettings s = root.getSettingsUseCase().run();
    ui_->pathLineEdit_->setText(QString::fromStdString(s.syncPath));
    ui_->cloudCheckSpinBox_->setValue(s.cloudCheckIntervalSec >= 5 && s.cloudCheckIntervalSec <= 3600 ? s.cloudCheckIntervalSec : 30);
    ui_->refreshSpinBox_->setValue(s.refreshIntervalSec >= 5 && s.refreshIntervalSec <= 3600 ? s.refreshIntervalSec : 60);
    ui_->hideToTrayCheckBox_->setChecked(s.hideToTray);
    ui_->closeToTrayCheckBox_->setChecked(s.closeToTray);
    connect(ui_->browseBtn_, &QPushButton::clicked, this, &SettingsDialog::onBrowseClicked);
    connect(ui_->saveBtn_, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);
    connect(ui_->cancelBtn_, &QPushButton::clicked, this, &QDialog::reject);
}

SettingsDialog::~SettingsDialog() {
    delete ui_;
}

void SettingsDialog::onBrowseClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose sync folder"),
                                                    ui_->pathLineEdit_->text());
    if (!dir.isEmpty())
        ui_->pathLineEdit_->setText(dir);
}

void SettingsDialog::onSaveClicked() {
    ydisquette::settings::AppSettings s = root_->getSettingsUseCase().run();
    s.syncPath = ui_->pathLineEdit_->text().trimmed().toStdString();
    s.cloudCheckIntervalSec = ui_->cloudCheckSpinBox_->value();
    s.refreshIntervalSec = ui_->refreshSpinBox_->value();
    s.hideToTray = ui_->hideToTrayCheckBox_->isChecked();
    s.closeToTray = ui_->closeToTrayCheckBox_->isChecked();
    root_->saveSettingsUseCase().run(s);
    JsonConfig c = JsonConfig::load();
    c.syncFolder = QString::fromStdString(s.syncPath);
    c.cloudCheckIntervalSec = s.cloudCheckIntervalSec;
    c.refreshIntervalSec = s.refreshIntervalSec;
    c.hideToTray = s.hideToTray;
    c.closeToTray = s.closeToTray;
    JsonConfig::save(c);
    accept();
}

}  // namespace ydisquette
