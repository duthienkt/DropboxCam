#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"               // SD Card ESP32
#include "SD_MMC.h"           // SD Card ESP32
#include "soc/soc.h"          // Disable brownour problems
#include "soc/rtc_cntl_reg.h" // Disable brownour problems
#include "driver/rtc_io.h"

#include "dropbox.h"
// Đây là key của app trong dropbox, mỗi app sẽ có 1 key riêng
#define DROPBOX_ACESS_KEY "pHBX-uh5eDAAAAAAAAALIN9cEeim_r2J3ScCnje-Y9txpvA50fqmNTuk9xiDKR-w"

RTC_DATA_ATTR int bootCount = 0;

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

int pictureNumber = 0;

#define uS_TO_S_FACTOR 1000000

#define WIFI_SSID "console.log(SSID)" // tên WiFi
#define WIFI_PWD "undefined"          // mật khẩu WiFi
#define TRIGGER_PIN 14                //đây là chân RX2(GPIO16), các chân còn lại đều là chân data, nếu sử dụng sẽ lỗi thẻ nhớ hoặc cam
#define TRIGGER_VALUE 1               //chụp khi chân  TRIGGER_PIN ở trạng thái cao(1), ngược lại gán 0
//Thông tin để cài đặt thời gian
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600 * 7;
const int daylightOffset_sec = 3600;

DropboxMan dropboxMan;
bool hasInternet = false;
bool hasInternetTime = false;
bool hasSDCard = false;

int frameCount = 0;
int frameRemain = 0;
bool hasMontion = false;
void capture();

void setupCamera()
{

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    pinMode(4, INPUT);
    digitalWrite(4, LOW);
    rtc_gpio_hold_dis(GPIO_NUM_4);

    if (psramFound())
    {
        config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
        config.jpeg_quality = 10;
        config.fb_count = 2;
    }
    else
    {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

    // Init Camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }
}

void setupWifi()
{
    int retry = 15;
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PWD);
    while (WiFi.status() != WL_CONNECTED && retry >= 0)
    {
        delay(500);
        Serial.print(".");
        retry--;
    }
    if (retry >= 0)
    {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        hasInternet = true;
    }
    else
    {
        hasInternet = false;
    }
}

void setupDropbox()
{
    dropboxMan.begin(DROPBOX_ACESS_KEY);
}

void setupSDCard()
{
    delay(500);
    Serial.println("Starting SD Card");
    if (!SD_MMC.begin())
    {
        Serial.println("SD Card Mount Failed");
        hasSDCard = false;
        return;
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE)
    {
        Serial.println("No SD Card attached");
        hasSDCard = false;
        return;
    }
    hasSDCard = true;
}

void setupTime()
{
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        hasInternetTime = false;
        return;
    }
    else
    {
        Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        hasInternetTime = true;
    }
}

void setupTrigger()
{
    if (TRIGGER_VALUE == 0)
    {
        pinMode(TRIGGER_PIN, INPUT_PULLUP);
    }
    else
    {
        pinMode(TRIGGER_PIN, INPUT_PULLDOWN);
    }
}

void setupFlash()
{
    if (!hasSDCard)
    {
        pinMode(4, OUTPUT);
        digitalWrite(4, LOW);
    }
}

void flashLight(bool on)
{
    if (!hasSDCard)
    {
        if (on)
        {
            digitalWrite(4, HIGH);
            delay(230);
        }
        else
        {
            digitalWrite(4, LOW);
        }
    }
}

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    pinMode(4, INPUT);
    digitalWrite(4, LOW);
    rtc_gpio_hold_dis(GPIO_NUM_4);
    setupTrigger();
    setupCamera();
    setupWifi();
    setupDropbox();
    setupSDCard();
    setupTime();
    setupFlash();
    delay(300);
}

void capture()
{
    camera_fb_t *fb = NULL;
    flashLight(true);
    fb = esp_camera_fb_get();
    flashLight(false);

    // digitalWrite(4, HIGH);

    // lấy giờ để đặt tên

    // tên file
    if (!fb)
    {
        Serial.println("Camera capture failed");
        return;
    }

    frameCount++;

    String path = "/picture/";
    if (hasInternetTime) // nếu có thể lấy giờ từ internet thì dùng nó để đặt tên, các trường hợp còn lại lấy thời gian chạy
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            path += String(millis());
        }
        else
        {
            path += "thang";
            if (timeinfo.tm_mon < 9)
                path += "0";
            path += String(timeinfo.tm_mon + 1); // vì tháng tính từ 0
            path += "ngay";
            if (timeinfo.tm_mday < 10)
                path += "0";
            path += String(timeinfo.tm_mday);
            path += "_";

            if (timeinfo.tm_hour < 10)
                path += "0";
            path += String(timeinfo.tm_hour);
            path += "g";
            if (timeinfo.tm_min < 10)
                path += "0";
            path += String(timeinfo.tm_min);
            path += "p";
            if (timeinfo.tm_sec < 10)
                path += "0";
            path += String(timeinfo.tm_sec);

            Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
        }
    }
    else
    {
        path += String(millis());
    }

    path += ".jpg";

    if (hasSDCard)
    {
        delay(1);
        fs::FS &fs = SD_MMC;
        Serial.printf("Picture file name: %s\n", path.c_str());

        File file = fs.open(path.c_str(), FILE_WRITE);
        if (!file)
        {
            Serial.println("Failed to open file in writing mode");
        }
        else
        {
            file.write(fb->buf, fb->len); // payload (image), payload length
            Serial.printf("Saved file to path: %s\n", path.c_str());
        }
        file.close();
    }
    if ((frameCount % 5 == 3 || !hasSDCard) && hasInternet)
        dropboxMan.bufferUpload(fb->buf, fb->len, path, 1); // tải lên tấm ở giữa
    esp_camera_fb_return(fb);
}

void loop()
{
    int d = digitalRead(TRIGGER_PIN);
    hasMontion = d == TRIGGER_VALUE;
    if (hasMontion)
    {
        if (hasSDCard)
        {
            frameRemain = 5; // tiếp tục chụp thêm 5 tấm nữa
        }
        else
        {
            frameRemain = 1;// không có thẻ thì chỉ chụp 1 tấm thôi
        }
        Serial.println("Trigger!");
    }
    if (frameRemain > 0) // cần chụp 
    {
        capture();
        frameRemain--; // giảm số tấm còn lại
        delay(100);    // để hình không bị chụp quá nhanh
    }
    else
    {
        frameCount = 0; // không còn thấy chuyển động, reset bộ đếm
    }
    delay(1);
}
