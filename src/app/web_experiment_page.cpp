#include "web_experiment_page.hpp"

namespace ydisquette {

WebExperimentPage::WebExperimentPage(QObject* parent) : QWebEnginePage(parent) {}

void WebExperimentPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString& message, int lineNumber, const QString& sourceID) {
    QWebEnginePage::javaScriptConsoleMessage(level, message, lineNumber, sourceID);
    emit javaScriptConsole(static_cast<int>(level), message, lineNumber, sourceID);
}

}  // namespace ydisquette
