#include "app/MainWindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("showeq-qt");
    app.setOrganizationName("showeq-unofficial");

    MainWindow w;
    w.show();

    return app.exec();
}
