#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    ui->secondaryGroupBox->hide();
    ui->TertiaryGroupBox->hide();
    this->layout()->setSizeConstraint(QLayout::SetFixedSize);
}

Dialog::~Dialog()
{
    delete ui;
}
