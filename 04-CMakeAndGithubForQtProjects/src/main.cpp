#include "hello.h"
#include <QCoreApplication>
#include <QDebug>
#include <iostream>

int main(int argc, char *argv[])
{
    /* Note: Usually the following line is what you would be putting here,
             but since all we want to do it output a message, we remove this.
    
             Otherwise, the app will invisibly running in the background, and you would have
             to go in an manually kill the process.
    */

    // TODO: Replace with QWidget-based GUI when you extend the template
    // QCoreApplication app(argc, argv);
    // return app.exec();

    std::cout << getHelloMessage().toStdString() << std::endl;
    return 0;
}
