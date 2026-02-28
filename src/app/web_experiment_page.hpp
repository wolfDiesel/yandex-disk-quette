#pragma once

#include <QString>
#include <QWebEnginePage>

namespace ydisquette {

class WebExperimentPage : public QWebEnginePage {
    Q_OBJECT
public:
    explicit WebExperimentPage(QObject* parent = nullptr);

signals:
    void javaScriptConsole(int level, const QString& message, int lineNumber, const QString& sourceId);

protected:
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString& message, int lineNumber, const QString& sourceID) override;
};

}  // namespace ydisquette
