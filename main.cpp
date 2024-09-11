#include <QApplication>
#include "mainwindow.h"
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/json.hpp>
#include <QDebug>

int main(int argc, char *argv[]) {
    // Initialize the MongoDB driver
    mongocxx::instance instance{};

    QApplication app(argc, argv);

    // Create and show the main window
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
