#ifndef WSREMOTECONTROL_H
#define WSREMOTECONTROL_H

#include <QWidget>
#include <QTimer>
#include <time.h>
#include "wsserialdevice.h"

#define TEMP_ONLINE                    (1)
#define PUMP_ONLINE                    (1<<1)
#define SALOR_USING                     (1<<2)
#define SALOR_WATER_ENTERING            (1<<3)
#define FIRE_TURNED_ON                  (1<<4)
#define FIRE_USING                      (1<<5)
#define TEMP_ERROR                      (1<<8)
#define PUMP_ERROR                      (2<<8)

#define ERROR_MASK                      0xff00
#define WORKING_STATUS_MASK             0x00ff

#define ENV_TEMP_MASK   0xff00
#define WATER_TEMP_MASK  0x00ff
#define ENV_TEMP_SHIFT   8
#define WATER_TEMP_SHIFT    0

#define LOG_INFO    2
#define LOG_DEBUG   4
#define LOG_ERROR   8
#define DATA_OUTPUT   16

#define ALL_OUTPUT_MASK 0x0f

#define OUTPUT_SEIRAL                       0
#define OUTPUT_AF_MESSAGE                   1     //Redirect the debug output

namespace Ui {
class WsRemoteControl;
}

typedef struct
{
  short          tempCalibration;      //Tempratrue calibration value
  byte           winterThreshold;      //The env temp decide winter
  byte           winterSwtichTemp;     //The center temp to switch between fire and gas in winter
  byte           summerSwitchTemp;     //The switch temp in summer
} waterSwichConfig_t;

class WsRemoteControl : public QWidget
{
    Q_OBJECT

public:
    explicit WsRemoteControl(QWidget *parent = 0);
    ~WsRemoteControl();

private slots:
    void on_uart_currentIndexChanged(const QString &arg1);
    void deviceError(QSerialPort::SerialPortError err);
    void printLogMessage(QString msg);
    void regularMonitor();

    void on_workModeButton_clicked();

    void on_heaterButton_clicked();
    void stringRead(QString);
    void cmdProcess(QByteArray cmd);

    void on_caliScrollBar_valueChanged(int value);

    void on_winterThresholdScrollBar_valueChanged(int value);

    void on_winterSwitchTempScrollBar_valueChanged(int value);

    void on_summerSwitchTempScrollBar_valueChanged(int value);

    void on_saveButton_clicked();

    void on_resetButton_clicked();

    void on_tabWidget_currentChanged(int index);

    void on_allCB_toggled(bool checked);

    void on_infoCB_toggled(bool checked);

    void on_dbgCB_toggled(bool checked);

    void on_errCB_toggled(bool checked);

    void on_outputCB_toggled(bool checked);

private:
    static const QString PREFERENCES_NAME;
    static const qint32 MAX_LOGS_VISIBLE;
    uint temperature;
    byte waterLevel;
    uint deviceStatus;
    byte workMode;
    byte waterSupplier;
    waterSwichConfig_t nvConfig;

    bool comInited = 0;
    bool coordinatorConnected=false;
    bool setDbgOpts = false;
    clock_t lastReceiveTime;
    WsSerialDevice *serialDevice = NULL;
    QTimer *timer = NULL;

    Ui::WsRemoteControl *ui;
    void initCmbCom();
    void loadPreferences();
    void savePreferences();
    void startMonitor();
    void stopMonitor();
    void populateNvConfig();
    void setDebugOpts(byte dbgOpt);
    byte readCurrentDebugOptsConfig();
    void saveDebugOpts(byte dbgOpt);
    void markSingleDebugOpt(byte mask, bool toggle);

signals:
    void logMessage(QString msg);
};

#endif // WSREMOTECONTROL_H
