#ifndef WSSERIALDEVICE_H
#define WSSERIALDEVICE_H

#include <QSerialPort>

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
    bool startSerialDevice(char channel);
    bool stopSerialDevice();

    QString errorMsg();
    QString strSerialError(QSerialPort::SerialPortError err);

signals:

private slots:
};

#endif // WSSERIALDEVICE_H
