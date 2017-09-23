/*
********************************************************************************
*                              COPYRIGHT NOTICE
*                             Copyright (c) 2016
*                             All rights reserved
*
*  @FileName       : sample.cpp
*  @Author         : scm 351721714@qq.com
*  @Create         : 2017/08/12 23:00:29
*  @Last Modified  : 2017/08/12 23:22:00
********************************************************************************
*/

#include "sample.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Sample smp(NULL);
    smp.show();
    return app.exec();
}
