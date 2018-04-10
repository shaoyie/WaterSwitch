#include "wsserialdevice.h"
#include <QDebug>

WsSerialDevice::WsSerialDevice(QObject *parent) :
    QSerialPort(parent)
{
}

WsSerialDevice::WsSerialDevice(QString name) :
    QSerialPort(name)
{
}

bool WsSerialDevice::open(OpenMode mode)
{
    bool ret = QSerialPort::open(mode);

    if (ret)
    {
        ret = (setBaudRate(QSerialPort::Baud115200) &&
               setDataBits(QSerialPort::Data8) &&
               setParity(QSerialPort::NoParity) &&
               setFlowControl(QSerialPort::NoFlowControl) &&
               setStopBits(QSerialPort::OneStop));
    }

    return ret;
}

bool WsSerialDevice::startSerialDevice(char channel)
{
    return (writeData(&channel, 1) == 1);
}

bool WsSerialDevice::stopSerialDevice()
{
    /* send any symbol that's not in range [11;26] */
    char stop = 1;

    return (writeData(&stop, 1) == 1);
}

QString WsSerialDevice::errorMsg()
{
    QSerialPort::SerialPortError err = QSerialPort::error();

    return strSerialError(err);
}

QString WsSerialDevice::strSerialError(QSerialPort::SerialPortError err)
{
    QString res;

    switch (err)
    {
    case QSerialPort::NoError: res = "No error"; break;
    case QSerialPort::DeviceNotFoundError: res = "Device not found"; break;
    case QSerialPort::PermissionError: res = "Permission denied"; break;
    case QSerialPort::OpenError: res = "Already open"; break;
    case QSerialPort::ParityError: res = "Device paritty error"; break;
    case QSerialPort::FramingError: res = "Device framing error"; break;
    case QSerialPort::ReadError: res = "Device read error"; break;
    case QSerialPort::WriteError: res = "Device write error"; break;
    case QSerialPort::ResourceError: res = "Device unplugged"; break;
    default: res = "Device error";
    };

    return res;
}
