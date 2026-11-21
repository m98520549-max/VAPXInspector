#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("VAPX Inspector");
    QApplication::setOrganizationName("VAPXTools");
    MainWindow window;
    window.show();
    return app.exec();
}
