#ifndef WSSERIALDEVICE_H
#define WSSERIALDEVICE_H

#include <QSerialPort>

#define UART_SOF                     0xFE

#define CMD0_READ 0
#define CMD0_READ_RSP 1
#define CMD0_WRITE 2
#define CMD0_WRITE_RSP 3

#define CMD1_TEMP  0
#define CMD1_OCCUPANCY 1
#define CMD1_DEVICE_STATUS 2
#define CMD1_WORK_MODE 3
#define CMD1_WATER_SUPPLIER 4
#define CMD1_SWITCH_OUTPUT 5
#define CMD1_NV_CONFIG 6

#define RPC_FRAME_HDR_SZ   3

#define RPC_POS_LEN        0
#define RPC_POS_CMD0       1
#define RPC_POS_CMD1       2
#define RPC_POS_DAT0       3

#define SOP_STATE      0x00
#define CMD_STATE1     0x01
#define CMD_STATE2     0x02
#define LEN_STATE      0x03
#define DATA_STATE     0x04
#define FCS_STATE      0x05

#define AUTO_CONTROL                    0x00
#define MANUAL_CONTROL                  0x01

#define SALOR_OFF                       0x00
#define SALOR_ON                        0x01

typedef unsigned char byte;

class WsSerialDevice : public QSerialPort
{
    Q_OBJECT
public:

    enum SnifferReadStatusE
    {
        SNIFFER_PACKET_AVAILABLE = 0,
        SNIFFER_BAD_HEADER = 1,
        SNIFFER_REPORT_ERROR = 2,
        SNIFFER_READ_FAIL = 3
    };

    explicit WsSerialDevice(QObject *parent = 0);
    explicit WsSerialDevice(QString name);
    /* Buffer size must be 128 bytes at least */
    bool open(OpenMode mode);
    bool queryDeviceStatus();
    bool sendSerialData(uint cmd0, uint cmd1, byte* data, byte len);
    void close();

    QString errorMsg();
    QString strSerialError(QSerialPort::SerialPortError err);

private:

    QByteArray tempUartBuf;
    int uartBufIndex = 0;
    int readDataStatus = SOP_STATE;
    int queryIndex = CMD1_TEMP;

    byte UartCalcFCS( byte *msg_ptr, byte len );

signals:
    void stringReady(QString);
    void cmdReady(QByteArray cmd);

private slots:
    void deviceReadAvailable();
};

#endif // WSSERIALDEVICE_H
