#include <QMessageBox>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QPlainTextEdit"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    textEdit(new QPlainTextEdit)
{
    ui->setupUi(this);
    this->setCentralWidget(textEdit);
}

MainWindow::~MainWindow()
{
    delete textEdit;
    delete ui;
}

void MainWindow::on_actionExit_triggered()
{
    if(textEdit->document()->isModified())
    {
        QMessageBox::StandardButton ret = QMessageBox::warning(
                this, tr("exit"), tr("you sure to exit?"),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        switch(ret)
        {
            case QMessageBox::Save:
                // save();
                break;
            case QMessageBox::Discard:
                break;
            default:
                return;
        }
    }
    this->close();
}
