#ifndef WSREMOTECONTROL_H
#define WSREMOTECONTROL_H

#include <QWidget>

namespace Ui {
class WsRemoteControl;
}

class WsRemoteControl : public QWidget
{
    Q_OBJECT

public:
    explicit WsRemoteControl(QWidget *parent = 0);
    ~WsRemoteControl();

private:
    static const QString PREFERENCES_NAME;

    Ui::WsRemoteControl *ui;
    void initCmbCom();
    void loadPreferences();
    void savePreferences();
};

#endif // WSREMOTECONTROL_H
