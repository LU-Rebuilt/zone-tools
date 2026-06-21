#include "main_window.h"

#include <QApplication>
#include <QDir>
#include <QSettings>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("LU-Rebuilt");
    app.setApplicationName("ZoneEditor");

    zone_editor::MainWindow window;
    window.show();

    if (argc > 1) {
        window.open_client_dir(argv[1]);
    } else {
        QSettings settings;
        QString last = settings.value("last_client_dir").toString();
        if (!last.isEmpty() && QDir(last).exists()) {
            window.open_client_dir(last.toStdString());
        }
    }

    return app.exec();
}
