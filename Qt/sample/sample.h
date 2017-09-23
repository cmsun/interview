/*
********************************************************************************
*                              COPYRIGHT NOTICE
*                             Copyright (c) 2016
*                             All rights reserved
*
*  @FileName       : sample.h
*  @Author         : scm 351721714@qq.com
*  @Create         : 2017/08/12 23:00:45
*  @Last Modified  : 2017/08/12 23:22:58
********************************************************************************
*/
#ifndef SAMPLE_H
#define SAMPLE_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>

class Sample : public QWidget
{
    Q_OBJECT

private:
   QPushButton *button;
   QLabel *label;

public:
   Sample(QWidget *parent = 0)
       : QWidget(parent)
   {
       button = new QPushButton(this);
       label = new QLabel("sample", this);
       label->setVisible(false);
       label->setAlignment(Qt::AlignCenter);
       QHBoxLayout *layout = new QHBoxLayout(this);
       layout->QLayout::addWidget(button);
       layout->QLayout::addWidget(label);
       this->setLayout(layout);
       this->resize(500, 50);
       connect(button, SIGNAL(clicked()), this, SLOT(toggleMessage()));
   }

public slots:
    void toggleMessage(void)
    {
        label->setVisible(!label->isVisible());
    }
};

#endif /* end of include guard: SAMPLE_H */
