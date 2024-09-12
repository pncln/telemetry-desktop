#include <QApplication>
#include "mainwindow.h"
#include <bsoncxx/json.hpp>
#include <QDebug>

int main(int argc, char *argv[]) {

    QApplication app(argc, argv);

    // Create and show the main window
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
