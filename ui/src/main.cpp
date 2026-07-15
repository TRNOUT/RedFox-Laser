#include <QApplication>

#include "MainWindow.hpp"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MainWindow window;
    window.resize(420, 200);
    window.show();
    return app.exec();
}
