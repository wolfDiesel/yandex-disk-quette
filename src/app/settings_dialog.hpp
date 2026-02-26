#pragma once

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

namespace ydisquette {

struct CompositionRoot;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(CompositionRoot& root, QWidget* parent = nullptr);
    ~SettingsDialog() override;

private slots:
    void onBrowseClicked();
    void onSaveClicked();

private:
    CompositionRoot* root_;
    Ui::SettingsDialog* ui_;
};

}  // namespace ydisquette
