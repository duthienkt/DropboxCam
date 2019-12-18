
#include "WiFi.h"
#include "WiFiClientSecure.h"
#include "FS.h"
#include "SD_MMC.h" // SD Card ESP32
#include "dropbox.h"

#define ver "v1.1.2"
#define debug_mode Serial

#define upload "/2/files/upload HTTP/1.1\r\n"
#define download "/2/files/download HTTP/1.1\r\n"
#define host_content "Host: content.dropboxapi.com\r\n"
#define content_type_octet "Content-Type: application/octet-stream\r\n"
#define _accept "Accept: */*\r\n"
#define Aut_Bearer "Authorization: Bearer "
#define timeout_client 10000
#define timeout_down 10000
#define content_API "162.125.5.8"
#define fingerprint_Content_API "9D 86 7B C9 7E 07 D7 5C 86 66 A3 E2 95 C3 B5 45 C5 1E 89 B3"
#define main_API "162.125.5.7"
#define fingerprint_Main_API "C8 50 54 26 17 A5 FB 9F 19 FC 59 ED 11 43 3C F9 37 F9 64 7F"
#define oauth2 "/oauth2/token HTTP/1.1\r\n"
#define host_API "Host: api.dropboxapi.com\r\n"
#define content_type_urlencoded "Content-Type: application/x-www-form-urlencoded\r\n"

DropboxMan::DropboxMan()
{
}

void DropboxMan::begin(String token)
{

    if (token.length() < 64)
    {

        debug_mode.println("FAIL: Small token!");

        return;
    }
    if (token.length() > 64)
    {

        debug_mode.println("FAIL: Big token!");

        return;
    }
    _token = token;
}

String DropboxMan::getToken(String code)
{

    if (code.length() < 43)
    {
        debug_mode.println("FAIL: Small code!");
        return "FAIL: Small code!";
    }
    if (code.length() > 43)
    {

        debug_mode.println("FAIL: Big code!");

        return "FAIL: Big code!";
    }

    WiFiClientSecure client;
    //WiFiClientSecure *client= new WiFiClientSecure;

    debug_mode.println("Connecting to Dropbox...");

    if (!client.connect(main_API, 443))
    {

        debug_mode.println("Connection failed!");

        //client.close();
        client.stop();
        return "FAIL";
    }

    if (client.verify(fingerprint_Main_API, "api.dropboxapi.com"))
    {

        debug_mode.println("Certificate matches!");
    }
    else
    {

        debug_mode.println("Certificate doesn't match!");
    }

    String pack = "code=" + code + "&grant_type=authorization_code&client_id=g0xkpmfu025pn8f&client_secret=knagh39wa75p8n9";
    client.print(String("POST ") + oauth2 +
                 host_API +
                 "User-Agent: ESP8266/Arduino_Dropbox_Manager_" + (String)ver + "\r\n" +
                 _accept +
                 content_type_urlencoded +
                 "Content-Length: " + pack.length() + "\r\n\r\n" +
                 pack);

    debug_mode.println("Request sent!");

    uint32_t millisTimeoutClient;
    millisTimeoutClient = millis();
    String line;
    while (client.connected() && ((millis() - millisTimeoutClient) < timeout_client))
    {
        line = client.readStringUntil('\n');
        if (line == "\r")
        {
            break;
        }
    }

    line = client.readStringUntil('\n');
    line = client.readStringUntil('\n');
    // client.close();
    client.stop();
    if (line.startsWith("{\"access_token\": \""))
    {

        debug_mode.println("Successfull!!!");
        debug_mode.println("Token:");
        debug_mode.print(line.substring(line.indexOf("{\"access_token\": \"") + 18, line.indexOf("\",")));

        return line.substring(line.indexOf("{\"access_token\": \"") + 18, line.indexOf("\","));
    }
    else
    {

        debug_mode.println(line);

        return "FAIL";
    }
}

bool DropboxMan::fileUpload(String localFile, String address, bool type)
{
    String _mode;
    if (type)
    {
        _mode = "overwrite";
    }
    else
    {
        _mode = "add";
    }
    WiFiClientSecure client;
    //WiFiClientSecure *client= new WiFiClientSecure;

    debug_mode.println("Connecting to Dropbox...");

    if (!client.connect(content_API, 443))
    {

        debug_mode.println("Connection failed!");

        //client.close();
        client.stop();
        return false;
    }

    if (client.verify(fingerprint_Content_API, "content.dropboxapi.com"))
    {

        debug_mode.println("Certificate matches!");
    }
    else
    {

        debug_mode.println("Certificate doesn't match!");
    }
    delay(50);

    fs::FS &fs = SD_MMC;
    File files = SD_MMC.open(localFile, "r+");
    uint32_t sizeFile = files.size();

    client.print(String("POST ") + upload +
                 host_content +
                 "User-Agent: ESP8266/Arduino_Dropbox_Manager_" + (String)ver + "\r\n" +
                 (String)Aut_Bearer + (String)_token + "\r\n" +
                 _accept +
                 "Dropbox-API-Arg: {\"path\": \"" + address + "\",\"mode\": \"" + _mode + "\",\"autorename\": true,\"mute\": false,\"strict_conflict\": false}\r\n" +
                 content_type_octet +
                 "Content-Length: " + sizeFile + "\r\n\r\n");
    uint32_t index = 0;
    char _buffer[1001];
    while (index < sizeFile)
    {
        yield();
        delay(100);
        index = index + 1000;
        files.readBytes(_buffer, 1000);
        _buffer[1000] = '\0';
        client.print(_buffer);
    }
    client.flush();
    files.close();

    debug_mode.println("Request sent!");

    uint32_t millisTimeoutClient;
    millisTimeoutClient = millis();
    String line;
    while (client.connected() && ((millis() - millisTimeoutClient) < timeout_client))
    {
        line = client.readStringUntil('\n');
        if (line == "\r")
        {
            break;
        }
    }

    line = client.readStringUntil('\n');
    line = client.readStringUntil('\n');
    //client.close();
    client.stop();
    //delete client;
    if (line.startsWith("{\"name\": \""))
    {

        debug_mode.println("Successfull!!!");
        debug_mode.println("File sent!");

        return true;
    }
    else
    {

        debug_mode.println("ERROR!!!");
        debug_mode.println(line);

        return false;
    }
}

bool DropboxMan::bufferUpload(byte *buff, int length, String address, bool type)
{
    String _mode;
    if (type)
    {
        _mode = "overwrite";
    }
    else
    {
        _mode = "add";
    }
    WiFiClientSecure client;
    //WiFiClientSecure *client= new WiFiClientSecure;
    debug_mode.println("Connecting to Dropbox...");
    if (!client.connect(content_API, 443))
    {
        debug_mode.println("Connection failed!");
        //client.close();
        client.stop();
        return false;
    }

    if (client.verify(fingerprint_Content_API, "content.dropboxapi.com"))
    {
        debug_mode.println("Certificate matches!");
    }
    else
    {

        debug_mode.println("Certificate doesn't match!");
    }
    client.print(String("POST ") + upload +
                 host_content +
                 "User-Agent: ESP8266/Arduino_Dropbox_Manager_" + (String)ver + "\r\n" +
                 (String)Aut_Bearer + (String)_token + "\r\n" +
                 _accept +
                 "Dropbox-API-Arg: {\"path\": \"" + address + "\",\"mode\": \"" + _mode + "\",\"autorename\": true,\"mute\": false}\r\n" +
                 content_type_octet +
                 "Content-Length: " + length + "\r\n\r\n");
    uint32_t index = 0;
    uint32_t remain;
    while (index < length)
    {
        remain = length - index;
        if (remain > 512)
            remain = 512;
        client.write(buff + index, remain);
        index += 512;
    }
    client.flush();

    debug_mode.println("Request sent!");

    uint32_t millisTimeoutClient;
    millisTimeoutClient = millis();
    String line;
    while (client.connected() && ((millis() - millisTimeoutClient) < timeout_client))
    {
        line = client.readStringUntil('\n');
        if (line == "\r")
        {
            break;
        }
    }

    line = client.readStringUntil('\n');
    Serial.println(line);
    line = client.readStringUntil('\n');
    Serial.println(line);
    //client.close();
    client.stop();
    //delete client;
    if (line.startsWith("{\"name\": \""))
    {

        debug_mode.println("Successfull!!!");
        debug_mode.println("File sent!");

        return true;
    }
    else
    {

        debug_mode.println("ERROR!!!");
        debug_mode.println(line);

        return false;
    }
}

bool DropboxMan::stringUpload(String data, String address, bool type)
{
    String _mode;
    if (type)
    {
        _mode = "overwrite";
    }
    else
    {
        _mode = "add";
    }
    WiFiClientSecure client;

    debug_mode.println("Connecting to Dropbox...");

    if (!client.connect(content_API, 443))
    {

        debug_mode.println("Connection failed!");

        return false;
    }

    if (client.verify(fingerprint_Content_API, "content.dropboxapi.com"))
    {

        debug_mode.println("Certificate matches!");
    }
    else
    {

        debug_mode.println("Certificate doesn't match!");
    }

    client.print(String("POST ") + upload +
                 host_content +
                 "User-Agent: ESP8266/Arduino_Dropbox_Manager_" + (String)ver + "\r\n" +
                 (String)Aut_Bearer + (String)_token + "\r\n" +
                 _accept +
                 "Dropbox-API-Arg: {\"path\": \"" + address + "\",\"mode\": \"" + _mode + "\",\"autorename\": true,\"mute\": false}\r\n" +
                 content_type_octet +
                 "Content-Length: " + data.length() + "\r\n\r\n" +
                 data);
    client.flush();

    debug_mode.println("Request sent!");

    uint32_t millisTimeoutClient;
    millisTimeoutClient = millis();
    String line;
    while (client.connected() && ((millis() - millisTimeoutClient) < timeout_client))
    {
        line = client.readStringUntil('\n');
        if (line == "\r")
        {
            break;
        }
    }
    line = client.readStringUntil('\n');
    line = client.readStringUntil('\n');
    //client.close();
    client.stop();
    if (line.startsWith("{\"name\": \""))
    {

        debug_mode.println("Successfull!!!");
        debug_mode.println("File sent!");

        return true;
    }
    else
    {

        debug_mode.println("ERROR!!!");
        debug_mode.println(line);

        return false;
    }
}
