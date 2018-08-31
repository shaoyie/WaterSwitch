#include <QtSerialPort/QSerialPortInfo>
#include <QProcess>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QIODevice>
#include "wsremotecontrol.h"
#include "ui_wsremotecontrol.h"

#define TIMER_INTERVAL  1000

#ifdef Q_OS_WIN32
// win
#define DISCONNECT_GAP  3000
#endif

#ifdef Q_OS_LINUX
// linux
#define DISCONNECT_GAP  40000
#endif

const QString WsRemoteControl::PREFERENCES_NAME = "pref";
const qint32 WsRemoteControl::MAX_LOGS_VISIBLE = 9999;

WsRemoteControl::WsRemoteControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WsRemoteControl)
{
    ui->setupUi(this);
    connect(this, SIGNAL(logMessage(QString)), this, SLOT(printLogMessage(QString)));

    initCmbCom();
    loadPreferences();
    comInited = 1;

    //Create Timer
    timer = new QTimer(this);
    timer->setInterval(TIMER_INTERVAL);
    connect(timer, SIGNAL(timeout()), this, SLOT(regularMonitor()));

    //Start the monitor
    startMonitor();
}

WsRemoteControl::~WsRemoteControl()
{
    stopMonitor();
    delete ui;
}

//Init the uart port list
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

void WsRemoteControl::on_uart_currentIndexChanged(const QString &arg1)
{
    if(comInited){
        savePreferences();
        startMonitor();
    }
}

void WsRemoteControl::startMonitor(){
    bool ret;

    //Stop current monitor first
    stopMonitor();

    //Create the UART device
    QString portName = (ui->uart->count() > 0 ?
                            ui->uart->itemData(ui->uart->currentIndex()).toString() : "");
    serialDevice = new WsSerialDevice(portName);

    qDebug() << "Open device";
    emit logMessage(QString("Open uart device at %1.").arg(serialDevice->portName()));
    //And open the port
    ret = serialDevice->open(QSerialPort::ReadWrite);
    if (!ret)
    {
        qDebug() << "Failed to open device";
        emit logMessage(QString("Can't open device at %1: %2.").arg(serialDevice->portName(), serialDevice->errorMsg()));
    }
    else
    {
        connect(serialDevice, SIGNAL(error(QSerialPort::SerialPortError)),
                this, SLOT(deviceError(QSerialPort::SerialPortError)));
        serialDevice->clear();
        connect(serialDevice, SIGNAL(stringReady(QString)), this, SLOT(stringRead(QString)));
        connect(serialDevice, SIGNAL(cmdReady(QByteArray)), this, SLOT(cmdProcess(QByteArray)));
        //Start the timer
        timer->start();
    }
}

void WsRemoteControl::stopMonitor(){
    if(timer->isActive()){
        //Stop monitor timer
        timer->stop();
    }
    if(serialDevice!=NULL){
        //Close the related uart port
        disconnect(serialDevice, 0, this, 0);
        serialDevice->close();
        serialDevice->deleteLater();
        serialDevice = NULL;
    }
}

void WsRemoteControl::stringRead(QString data){
    printLogMessage(data);
}

void WsRemoteControl::cmdProcess(QByteArray cmd){
    byte cmd0=cmd[RPC_POS_CMD0];
    byte cmd1=cmd[RPC_POS_CMD1];
    byte length=cmd[RPC_POS_LEN];
    int temp;
    lastReceiveTime = clock();

    switch(cmd1){
    case CMD1_TEMP:
        temperature = *((short*)&(cmd.data()[RPC_POS_DAT0]));
        //Environment temperature
        temp = (temperature&ENV_TEMP_MASK)>>ENV_TEMP_SHIFT;
        if(temp>99){
            temp = 99;
        }
        ui->envTemp->display(temp);
        //Water temperature
        if(deviceStatus & TEMP_ONLINE) {
            temp = (temperature&WATER_TEMP_MASK)>>WATER_TEMP_SHIFT;
            if(temp>99){
                temp = 99;
            }
            ui->salorTemp->display(temp);
        }
        break;
    case CMD1_OCCUPANCY:
        waterLevel = cmd[RPC_POS_DAT0];
        switch(waterLevel){
        case 0:
            ui->salorIcon->setPixmap(QPixmap(":/icons/salor0.png"));
            break;
        case 1:
            ui->salorIcon->setPixmap(QPixmap(":/icons/salor1.png"));
            break;
        case 2:
            ui->salorIcon->setPixmap(QPixmap(":/icons/salor2.png"));
            break;
        case 3:
            ui->salorIcon->setPixmap(QPixmap(":/icons/salor3.png"));
            break;
        case 4:
            ui->salorIcon->setPixmap(QPixmap(":/icons/salor4.png"));
            break;
        default:
            break;
        }
        break;
    case CMD1_DEVICE_STATUS:
        deviceStatus = *((short*)&(cmd.data()[RPC_POS_DAT0]));
        if(deviceStatus&SALOR_USING){
            //salor water is using
            ui->downWaterIcon->setPixmap(QPixmap(":/icons/down_water.png"));
        } else {
            ui->downWaterIcon->setPixmap(QPixmap(":/icons/down_no.png"));
        }
        if(deviceStatus&SALOR_WATER_ENTERING){
            //salor water is entering
            ui->upWaterIcon->setPixmap(QPixmap(":/icons/up_water.png"));
        } else {
            ui->upWaterIcon->setPixmap(QPixmap(":/icons/up_no.png"));
        }
        if(deviceStatus&FIRE_USING){
            //fire heater is using
            ui->fireIcon->setPixmap(QPixmap(":/icons/fire_r.png"));
        } else if(deviceStatus&FIRE_TURNED_ON){
            //fire heater is turned on
            ui->fireIcon->setPixmap(QPixmap(":/icons/fire_b.png"));
        } else {
            //fire heater is off
            ui->fireIcon->setPixmap(QPixmap(":/icons/fire_g.png"));
        }
        if(deviceStatus&TEMP_ERROR){
            //the temperature sensor has error
            //ui->tempAlarm->setPixmap(QPixmap(":/icons/alarm.png"));
            ui->tempAlarm->show();
        } else {
            //the temperature sensor works
            ui->tempAlarm->hide();
        }
        if(deviceStatus&PUMP_ERROR){
            //the pump sensor has error
            //ui->pumpAlarm->setPixmap(QPixmap(":/icons/alarm.png"));
            ui->pumpAlarm->show();
        } else {
            //the pump sensor works
            ui->pumpAlarm->hide();
        }
        break;
    case CMD1_WORK_MODE:
        workMode = cmd[RPC_POS_DAT0];
        if(workMode==AUTO_CONTROL){
            ui->workModeLabel->setText(tr("Auto"));
        } else if(workMode==MANUAL_CONTROL){
            ui->workModeLabel->setText(tr("Manual"));
        } else{
            ui->workModeLabel->setText("");
        }
        break;
    case CMD1_WATER_SUPPLIER:
        waterSupplier = cmd[RPC_POS_DAT0];

        if(waterSupplier==SALOR_OFF){
            //ui->fireSelector->setPixmap(QPixmap(":/icons/selector.png"));
            ui->fireSelector->show();
            ui->salorSelector->hide();
        } else if(waterSupplier==MANUAL_CONTROL){
            ui->fireSelector->hide();
            ui->salorSelector->show();
        } else{
            ui->fireSelector->hide();
            ui->salorSelector->hide();
        }
        break;
    case CMD1_SWITCH_OUTPUT:
        setDebugOpts(cmd[RPC_POS_DAT0]);
        break;
    case CMD1_NV_CONFIG:
        if(cmd0 == CMD0_READ_RSP){
            //It's we requested to read it
            memcpy(&nvConfig, &(cmd.data()[RPC_POS_DAT0]), 5);
            populateNvConfig();
        } else if(cmd0 == CMD0_WRITE_RSP){
            //We write done
        }
        break;
    default:
        break;
    }
}

void WsRemoteControl::deviceError(QSerialPort::SerialPortError err)
{
    emit logMessage(QString("Sniffer device error %1").arg(serialDevice->strSerialError(err)));
    disconnect(serialDevice, SIGNAL(readyRead()), this, SLOT(deviceReadAvailable()));
    disconnect(serialDevice, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(deviceError(QSerialPort::SerialPortError)));
}


void WsRemoteControl::printLogMessage(QString msg)
{
    if (ui->logBrowser->document()->blockCount() == MAX_LOGS_VISIBLE)
    {
        ui->logBrowser->clear();
    }
    ui->logBrowser->append(msg);
}

void WsRemoteControl::regularMonitor(){
    //Regular query the system's status
    serialDevice->queryDeviceStatus();
    clock_t time = clock();
    clock_t gap = time-(lastReceiveTime);
    if(gap>DISCONNECT_GAP){
        //3 seconds didn't get response
        coordinatorConnected = false;
        ui->fireIcon->hide();
        ui->salorIcon->hide();
    } else {
        coordinatorConnected = true;
        ui->fireIcon->show();
        ui->salorIcon->show();
    }

}

void WsRemoteControl::on_workModeButton_clicked()
{
    if(serialDevice!=NULL && coordinatorConnected){
        byte workModeL = !workMode;
        serialDevice->sendSerialData(CMD0_WRITE, CMD1_WORK_MODE, &workModeL, 1);
    }
}

void WsRemoteControl::on_heaterButton_clicked()
{
    if(serialDevice!=NULL && coordinatorConnected){
        byte supplier=!waterSupplier;
        serialDevice->sendSerialData(CMD0_WRITE, CMD1_WATER_SUPPLIER, &supplier, 1);
    }
}

void WsRemoteControl::on_caliScrollBar_valueChanged(int value)
{
    ui->caliLabel->setText(QString::number(value));
}

void WsRemoteControl::on_winterThresholdScrollBar_valueChanged(int value)
{
    ui->winterThresholdLabel->setText(QString::number(value));
}

void WsRemoteControl::on_winterSwitchTempScrollBar_valueChanged(int value)
{
    ui->winterSwitchTempLabel->setText(QString::number(value));
}

void WsRemoteControl::on_summerSwitchTempScrollBar_valueChanged(int value)
{
    ui->summerSwitchTempLabel->setText(QString::number(value));
}

void WsRemoteControl::populateNvConfig(){

    //Calibration
    ui->caliLabel->setText(QString::number(nvConfig.tempCalibration));
    ui->caliScrollBar->setValue(nvConfig.tempCalibration);
    //Winter threshold
    ui->winterThresholdLabel->setText(QString::number(nvConfig.winterThreshold));
    ui->winterThresholdScrollBar->setValue(nvConfig.winterThreshold);
    //Winter switch temp
    ui->winterSwitchTempLabel->setText(QString::number(nvConfig.winterSwtichTemp));
    ui->winterSwitchTempScrollBar->setValue(nvConfig.winterSwtichTemp);
    //Summer switch temp
    ui->summerSwitchTempLabel->setText(QString::number(nvConfig.summerSwitchTemp));
    ui->summerSwitchTempScrollBar->setValue(nvConfig.summerSwitchTemp);
}

void WsRemoteControl::on_saveButton_clicked()
{
    waterSwichConfig_t nvConfigSave;
    nvConfigSave.tempCalibration=ui->caliScrollBar->value();
    nvConfigSave.winterThreshold=ui->winterThresholdScrollBar->value();
    nvConfigSave.winterSwtichTemp=ui->winterSwitchTempScrollBar->value();
    nvConfigSave.summerSwitchTemp=ui->summerSwitchTempScrollBar->value();
    if(serialDevice!=NULL){
        serialDevice->sendSerialData(CMD0_WRITE, CMD1_NV_CONFIG, (byte*)&nvConfigSave, 5);
    }
}

void WsRemoteControl::on_resetButton_clicked()
{
    populateNvConfig();
}

void WsRemoteControl::on_tabWidget_currentChanged(int index)
{
    if(serialDevice!=NULL){
        if(index==1){
            serialDevice->sendSerialData(CMD0_READ, CMD1_NV_CONFIG, NULL, 0);
        } else if(index==2){
            serialDevice->sendSerialData(CMD0_READ, CMD1_SWITCH_OUTPUT, NULL, 0);
        }
    }
}

void WsRemoteControl::on_allCB_toggled(bool checked)
{
    if(!setDbgOpts){
        //Only operate when we are not is a big config
        if(checked){
            setDebugOpts(ALL_OUTPUT_MASK);
            saveDebugOpts(ALL_OUTPUT_MASK);
        } else {
            setDebugOpts(0);
            saveDebugOpts(0);
        }
    }
}

void WsRemoteControl::on_infoCB_toggled(bool checked)
{
    markSingleDebugOpt(LOG_INFO, checked);
}

void WsRemoteControl::on_dbgCB_toggled(bool checked)
{
    markSingleDebugOpt(LOG_DEBUG, checked);
}

void WsRemoteControl::on_errCB_toggled(bool checked)
{
    markSingleDebugOpt(LOG_ERROR, checked);
}

void WsRemoteControl::on_outputCB_toggled(bool checked)
{
    markSingleDebugOpt(OUTPUT_AF_MESSAGE, checked);
}

void WsRemoteControl::markSingleDebugOpt(byte mask, bool toggle){
    if(!setDbgOpts){
        //Only operate when we are not is a big config
        byte opts = readCurrentDebugOptsConfig();
        if(toggle){
            opts|=mask;
        } else {
            opts &= (~mask);
        }
        //All selected?
        setDebugOpts(opts);
        saveDebugOpts(opts);
    }
}

void WsRemoteControl::setDebugOpts(byte dbgOpt){
    //We are setting it, should write back before we are done
    setDbgOpts = true;
    //The different output levels
    if(dbgOpt & LOG_INFO){
        ui->infoCB->setChecked(true);
    } else {
        ui->infoCB->setChecked(false);
    }
    if(dbgOpt & LOG_DEBUG){
        ui->dbgCB->setChecked(true);
    } else {
        ui->dbgCB->setChecked(false);
    }
    if(dbgOpt & LOG_ERROR){
        ui->errCB->setChecked(true);
    } else {
        ui->errCB->setChecked(false);
    }
    //Control the output
    if(dbgOpt & OUTPUT_AF_MESSAGE){
        ui->outputCB->setChecked(true);
    } else {
        ui->outputCB->setChecked(false);
    }
    //All selected?
    if((dbgOpt&ALL_OUTPUT_MASK)^ALL_OUTPUT_MASK){
        ui->allCB->setChecked(false);
    } else {
        ui->allCB->setChecked(true);
    }
    //We are done, can write back now
    setDbgOpts = false;
}

byte WsRemoteControl::readCurrentDebugOptsConfig(){
    byte dbgOpts=0;
    if(ui->allCB->isChecked()){
        //If all is checked, then select all
        dbgOpts|=ALL_OUTPUT_MASK;
    } else {
        if(ui->infoCB->isChecked()){
            //Info output
            dbgOpts|=LOG_INFO;
        }
        if(ui->dbgCB->isChecked()){
            //Debug output
            dbgOpts|=LOG_DEBUG;
        }
        if(ui->errCB->isChecked()){
            //Error output
            dbgOpts|=LOG_ERROR;
        }
        if(ui->outputCB->isChecked()){
            //Output on AF message
            dbgOpts|=OUTPUT_AF_MESSAGE;
        }
    }
    return dbgOpts;
}

void WsRemoteControl::saveDebugOpts(byte dbgOpt){

    if((!setDbgOpts) && serialDevice!=NULL){
        serialDevice->sendSerialData(CMD0_WRITE, CMD1_SWITCH_OUTPUT, &dbgOpt, 1);
    }
}
