#include <QApplication>
#include <QStyleFactory>
#include "mainwindow.h"
#include "database.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 使用 Fusion 主题，更现代的外观
    app.setStyle(QStyleFactory::create("Fusion"));

    // 深色调色板
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window,          QColor(53,  53,  53));
    darkPalette.setColor(QPalette::WindowText,      Qt::white);
    darkPalette.setColor(QPalette::Base,            QColor(35,  35,  35));
    darkPalette.setColor(QPalette::AlternateBase,   QColor(53,  53,  53));
    darkPalette.setColor(QPalette::ToolTipBase,     QColor(25,  25,  25));
    darkPalette.setColor(QPalette::ToolTipText,     Qt::white);
    darkPalette.setColor(QPalette::Text,            Qt::white);
    darkPalette.setColor(QPalette::Button,          QColor(53,  53,  53));
    darkPalette.setColor(QPalette::ButtonText,      Qt::white);
    darkPalette.setColor(QPalette::BrightText,      Qt::red);
    darkPalette.setColor(QPalette::Link,            QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight,       QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);
    // ★★★ 新增：初始化数据库 ★★★
    Database::instance().init();
    MainWindow w;
    w.show();
    return app.exec();
}