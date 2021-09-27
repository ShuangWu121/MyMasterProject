#ifndef EVOTING_H
#define EVOTING_H

#include <QWidget>

namespace Ui {
class eVoting;
}

class eVoting : public QWidget
{
    Q_OBJECT

public:
    explicit eVoting(QWidget *parent = 0);
    ~eVoting();

private:
    Ui::eVoting *ui;
};

#endif // EVOTING_H
