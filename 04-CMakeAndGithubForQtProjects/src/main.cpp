#include "hello.h"
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << getHelloMessage();

    // TODO: Replace with QWidget-based GUI when you extend the template
    return app.exec();
}