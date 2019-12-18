#ifndef Dropbox_h
#define Dropbox_h

#include "WiFiClientSecure.h"

class DropboxMan
{
public:
    DropboxMan();
    void begin(String token);
    String getToken(String code);
    bool fileUpload(String localFile, String address, bool type);
    bool bufferUpload(byte * buff,int length, String address, bool type);
    bool stringUpload(String data, String address, bool type);

private:
    String _token;
};

#endif