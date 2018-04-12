#include "wsserialdevice.h"
#include <QDebug>

WsSerialDevice::WsSerialDevice(QObject *parent) :
    QSerialPort(parent)
{
}

WsSerialDevice::WsSerialDevice(QString name) :
    QSerialPort(name)
{
    tempUartBuf = QByteArray("");
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
        clear();
        connect(this, SIGNAL(readyRead()), this, SLOT(deviceReadAvailable()));
    }

    return ret;
}

void WsSerialDevice::close(){
    disconnect(this, 0, 0, 0);
    QSerialPort::close();
}

//Send the given data through the serial port
bool WsSerialDevice::sendSerialData(uint cmd0, uint cmd1, byte* data, byte len){
    QByteArray pbuf;
    int i=0;
    int readIndex=0;
    //pbuf=(byte*)malloc(len + 5);
    pbuf.resize(len + 5);
    bool ret = 0;

    //Fill the header
    pbuf[i++]=UART_SOF;
    pbuf[i++]=len;
    pbuf[i++]=cmd0;
    pbuf[i++]=cmd1;
    //Copy the data
    while(readIndex<len){
        pbuf[i++]=data[readIndex++];
    }
    pbuf[i]=UartCalcFCS((byte *)&(pbuf.data()[1]), len+3);
    //const char* obuf=(char*)pbuf;
    ret = (writeData(pbuf.data(), len+5) == (len+5));
    return ret;
}

byte WsSerialDevice::UartCalcFCS( byte*msg_ptr, byte len )
{
    byte x;
    byte xorResult;

    xorResult = 0;

    for ( x = 0; x < len; x++, msg_ptr++ )
        xorResult = xorResult ^ *msg_ptr;

    return ( xorResult );
}

bool WsSerialDevice::queryDeviceStatus(){
    bool ret;
    ret = sendSerialData(CMD0_READ, queryIndex++, NULL, 0);
    if(queryIndex>CMD1_WATER_SUPPLIER){
        queryIndex = CMD1_TEMP;
    }
    return ret;
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


void WsSerialDevice::deviceReadAvailable()
{

    QByteArray packet = readAll();
    quint32 len = packet.length();
    int index=0;

    if (len == 0)
    {
        qDebug()<<"Device read error";
        return;
    }

    byte ch;

    while (index < len)
    {
        ch = packet[index++];
        switch (readDataStatus)
        {
        case SOP_STATE:
            if (ch == UART_SOF) {
                //Find the potential cmd start
                readDataStatus = LEN_STATE;
                if(tempUartBuf.size()>0){
                    //Already stored some data, then output them
                    QString data = tempUartBuf.data();
                    emit stringReady(data);
                    tempUartBuf.clear();
                }
                //TODO: Here we throw the SOF if later we found it's not a valid cmd package.
                //If needed, find it back
            } else {
                //Store the current data
                tempUartBuf.append(ch);
            }
            break;

        case LEN_STATE:
            tempUartBuf.append(ch);
            readDataStatus = CMD_STATE1;
            break;

        case CMD_STATE1:
            tempUartBuf.append(ch);
            readDataStatus = CMD_STATE2;
            break;

        case CMD_STATE2:
            tempUartBuf.append(ch);
            /* If there is no data, skip to FCS state */
            if ((byte)(tempUartBuf[RPC_POS_LEN])>0)
            {
                readDataStatus = DATA_STATE;
            }
            else
            {
                readDataStatus = FCS_STATE;
            }
            break;

        case DATA_STATE:

            /* Fill in the buffer the first byte of the data */
            tempUartBuf.append(ch);
            while(index<len && tempUartBuf.size()<tempUartBuf[RPC_POS_LEN]+3){
                ch = packet[index++];
                tempUartBuf.append(ch);
            }

            /* If number of bytes read is equal to data length, time to move on to FCS */
            if ( tempUartBuf.size()==tempUartBuf[RPC_POS_LEN]+3 )
                readDataStatus = FCS_STATE;

            break;

        case FCS_STATE:

            /* Make sure it's correct */
            if ((UartCalcFCS ((byte*)tempUartBuf.data(), RPC_FRAME_HDR_SZ + tempUartBuf[RPC_POS_LEN]) == ch))
            {
                emit cmdReady(tempUartBuf);
                tempUartBuf.clear();
            }
            else
            {
                //Need do nothing, just return to SOP_STATE and record the data for maybe future output
                tempUartBuf.append(ch);
            }

            /* Reset the state, send or discard the buffers at this point */
            readDataStatus = SOP_STATE;

            break;

        default:
            break;
        }
    }

    if (readDataStatus == SOP_STATE){
        if(tempUartBuf.size()>0){
            //Already stored some data, then output them
            QString data = tempUartBuf.data();
            emit stringReady(data);
            tempUartBuf.clear();
        }
    }
}
