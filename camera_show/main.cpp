#include "camera_show.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
        QApplication a(argc, argv);
        camera_show w(NULL, argv[1]);
        w.show();
        return a.exec();
}
