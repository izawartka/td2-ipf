#include "Serial.h"

Serial::Serial(char *portName)
{
    this->connected = false;

    this->hSerial = CreateFileA(static_cast<LPCSTR>(portName),
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
    if (this->hSerial == INVALID_HANDLE_VALUE){
        if (GetLastError() == ERROR_FILE_NOT_FOUND){
            printf("BLAD: Uruchomienie nie powiodlo sie. Powod: %s niedostepny\n", portName);
        }
    else
        {
            printf("BLAD!!!");
        }
    }
    else {
        DCB dcbSerialParameters = {0};

        if (!GetCommState(this->hSerial, &dcbSerialParameters)) {
            printf("uzyskiwanie parametrow portu szeregowego nie powiodlo sie");
        }
        else {
            dcbSerialParameters.BaudRate = CBR_9600;
            dcbSerialParameters.ByteSize = 8;
            dcbSerialParameters.StopBits = ONESTOPBIT;
            dcbSerialParameters.Parity = NOPARITY;
            dcbSerialParameters.fDtrControl = DTR_CONTROL_ENABLE;

            if (!SetCommState(hSerial, &dcbSerialParameters))
            {
                printf("UWAGA: ustawianie parametrow portu szeregowego nie powiodlo sie\n");
            }
            else {
                this->connected = true;
                PurgeComm(this->hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
                Sleep(ARDUINO_WAIT_TIME);
            }
        }
    }
}

Serial::~Serial()
{
    if (this->connected){
        this->connected = false;
        CloseHandle(this->hSerial);
    }
}

int Serial::ReadData(char *buffer, unsigned int buf_size)
{
    DWORD bytesRead;
    unsigned int toRead = 0;

    ClearCommError(this->hSerial, &this->errors, &this->status);

    if (this->status.cbInQue > 0){
        if (this->status.cbInQue > buf_size){
            toRead = buf_size;
        }
        else toRead = this->status.cbInQue;
    }

    if (ReadFile(this->hSerial, buffer, toRead, &bytesRead, NULL)) return bytesRead;

    return 0;
}

bool Serial::WriteData(const char *buffer, unsigned int buf_size)
{
    DWORD bytesSend;

    if (!WriteFile(this->hSerial, (void*) buffer, buf_size, &bytesSend, 0)){
        ClearCommError(this->hSerial, &this->errors, &this->status);
        return false;
    }
    else return true;
}

bool Serial::IsConnected()
{
    return this->connected;
}
