#include "evoting.h"
#include "ui_evoting.h"

eVoting::eVoting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::eVoting)
{
    ui->setupUi(this);
}

eVoting::~eVoting()
{
    delete ui;
}
