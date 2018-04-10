#include <QtSerialPort/QSerialPortInfo>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QIODevice>
#include "wsremotecontrol.h"
#include "ui_wsremotecontrol.h"

const QString WsRemoteControl::PREFERENCES_NAME = "pref";

WsRemoteControl::WsRemoteControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WsRemoteControl)
{
    ui->setupUi(this);
    initCmbCom();
}

WsRemoteControl::~WsRemoteControl()
{
    delete ui;
}

//void WsRemoteControl::initControls(){
//    ui->fireIcon->setPixmap(QPixmap(":/icons/fire_r.png"));
//}

void WsRemoteControl::initCmbCom()
{
    ui->uart->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        ui->uart->addItem(QString("%1 (%2)").arg(info.description(), info.portName()),
                            QVariant(info.portName()));
    }
}

void WsRemoteControl::loadPreferences()
{
    QFile *pref = new QFile(PREFERENCES_NAME);

    if (pref->exists())
    {
        if (pref->open(QIODevice::ReadOnly))
        {
            QTextStream in(pref);
            in.setCodec("UTF-8");

            QString tmp = in.readLine();

            if (!tmp.isNull())
            {
                int index = ui->uart->findData(tmp);

                if (index != -1)
                {
                    ui->uart->setCurrentIndex(index);
                }
            }

            pref->close();
        }
    }
}

void WsRemoteControl::savePreferences()
{
    QFile *pref = new QFile(PREFERENCES_NAME);

    if (pref->open(QIODevice::WriteOnly))
    {
        QTextStream out(pref);
        out.setCodec("UTF-8");

//        out << prefferedWs;
//        out << "\n";
//        out << prefferedPcap;
//        out << "\n";
//        out << (uchar)ui->cmbChannel->itemData(ui->cmbChannel->currentIndex()).toUInt();
//        out << "\n";
        out << ui->uart->itemData(ui->uart->currentIndex()).toString();

        pref->close();
    }
}
