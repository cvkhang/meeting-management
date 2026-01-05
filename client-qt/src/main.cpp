#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application info
    app.setApplicationName("Meeting Management");
    app.setApplicationVersion("1.0");
    
    // Set Vietnamese locale
    QLocale::setDefault(QLocale(QLocale::Vietnamese, QLocale::Vietnam));
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    return app.exec();
}
