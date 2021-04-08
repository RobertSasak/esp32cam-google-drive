#include "config.h"
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"
#include "esp_camera.h"
// Time
#include "time.h"
#include "lwip/err.h"
#include "lwip/apps/sntp.h"
// MicroSD
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
//
#include <EEPROM.h>
// OTA
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

const char *folder = FOLDER;
const char *scriptID = SCRIPT_ID;
const char *wifiNetwork = WIFI_NETWORK;
const char *password = PASSWORD;
const char *host = HOST;
const int port = PORT;
const int sleepTime = SLEEP_TIME;
const int sleepTimeNight = SLEEP_TIME_NIGHT;
const int nightStart = NIGHT_START;
const int nightEnd = NIGHT_END;
const int waitingTime = WAITING_TIME;
const int otaWaitingTime = OTA_WAITING_TIME;

#define CAMERA_MODEL_AI_THINKER

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

bool isNight = false;

bool connectWifi()
{
  WiFi.mode(WIFI_STA);

  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(wifiNetwork);
  WiFi.begin(wifiNetwork, password);
  int retry = 20;
  while (WiFi.status() != WL_CONNECTED && retry > 0)
  {
    Serial.print(".");
    delay(500);
    retry--;
  }
  return WiFi.status() == WL_CONNECTED;
}

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);

  if (connectWifi())
  {
    ota();
  }

  camera_fb_t *fb = getPicture();

  if (WiFi.status() == WL_CONNECTED)
  {
    initTime();
    sendPhotoDrive(fb);
  }
  else
  {
    Serial.println("Cannot connect to Wifi");
  }

  esp_err_t err = initSDCard();
  if (err == ESP_OK)
  {
    Serial.println("SD card mount successfully!");
    savePhoto(fb);
  }
  else
  {
    Serial.printf("Failed to mount SD card VFAT filesystem. Error: %s\n", esp_err_to_name(err));
  }
  esp_camera_fb_return(fb);
  deepSleep();
}

void loop()
{
}

esp_err_t initCamera()
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
  if (psramFound())
  {
    Serial.println("PSRAM found");
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  return esp_camera_init(&config);
}

void sendPhotoDrive(camera_fb_t *fb)
{
  Serial.println("Connect to " + String(host));
  WiFiClientSecure client;
  client.setInsecure();
  if (client.connect(host, port))
  {
    Serial.println("Connection successful");
    Serial.println("Sending image to Google Drive.");
    Serial.println("Size: " + String(fb->len) + "byte");

    String url = "/macros/s/" + String(scriptID) + "/exec?folder=" + String(folder);

    client.println("POST " + url + " HTTP/1.1");
    client.println("Host: " + String(host));
    client.println("Transfer-Encoding: chunked");
    client.println();

    int fbLen = fb->len;
    char *input = (char *)fb->buf;
    int chunkSize = 3 * 1000; // must be multiple of 3
    int chunkBase64Size = base64_enc_len(chunkSize);
    char output[chunkBase64Size + 1];

    Serial.println();
    int chunk = 0;
    for (int i = 0; i < fbLen; i += chunkSize)
    {
      int l = base64_encode(output, input, min(fbLen - i, chunkSize));
      client.print(l, HEX);
      client.print("\r\n");
      client.print(output);
      client.print("\r\n");
      delay(100);
      input += chunkSize;
      Serial.print(".");
      chunk++;
      if (chunk % 50 == 0)
      {
        Serial.println();
      }
    }
    client.print("0\r\n");
    client.print("\r\n");

    Serial.println("Waiting for response.");
    long int StartTime = millis();
    while (!client.available())
    {
      Serial.print(".");
      delay(100);
      if ((StartTime + waitingTime * 1000) < millis())
      {
        Serial.println();
        Serial.println("No response.");
        break;
      }
    }
    Serial.println();
    while (client.available())
    {
      Serial.print(char(client.read()));
    }
  }
  else
  {
    Serial.println("Connected to " + String(host) + " failed.");
  }
  client.stop();
}

void initTime()
{
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_init();
  // wait for time to be set
  time_t now = 0;
  struct tm timeinfo;
  timeinfo = {0};
  int retry = 0;
  const int retry_count = 10;
  while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count)
  {
    Serial.printf("Waiting for system time to be set... (%d/%d)\n", retry, retry_count);
    delay(2000);
    time(&now);
    localtime_r(&now, &timeinfo);
  }
  setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  tzset();

  if (timeinfo.tm_hour >= nightStart || timeinfo.tm_hour < nightEnd)
  {
    isNight = true;
  }
}

static esp_err_t initSDCard()
{
  esp_err_t ret = ESP_FAIL;
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 1,
  };
  sdmmc_card_t *card;

  Serial.println("Mounting SD card...");
  return esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
}

static esp_err_t savePhotoToSDCard(char *filename, camera_fb_t *fb)
{
  Serial.println(filename);
  char *path = (char *)malloc(150);
  sprintf(path, "/sdcard/%s", folder);
  mkdir(path, 0777);
  free(path);
  FILE *file = fopen(filename, "w");
  if (file != NULL)
  {
    size_t err = fwrite(fb->buf, 1, fb->len, file);
    Serial.printf("File saved: %s\n", filename);
  }
  else
  {
    Serial.println("Could not open file");
  }
  fclose(file);
  esp_camera_fb_return(fb);
}

void writeIntIntoEEPROM(int address, int number)
{
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}

int readIntFromEEPROM(int address)
{
  byte byte1 = EEPROM.read(address);
  byte byte2 = EEPROM.read(address + 1);
  return (byte1 << 8) + byte2;
}

void savePhoto(camera_fb_t *fb)
{
  char *filename = (char *)malloc(150);
  struct tm timeinfo;
  time_t now;
  time(&now);
  localtime_r(&now, &timeinfo);
  if (timeinfo.tm_year < (2016 - 1900))
  { // if no internet or time not set
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    int randomNumber;
    int fileNumber = 1;
    EEPROM.begin(4);
    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
    {
      randomNumber = readIntFromEEPROM(0);
      fileNumber = readIntFromEEPROM(2);
      fileNumber++;
      writeIntIntoEEPROM(2, fileNumber);
    }
    else
    {
      randomNumber = random(10000, 99999);
      writeIntIntoEEPROM(0, randomNumber);
      writeIntIntoEEPROM(2, fileNumber);
    }
    if (!EEPROM.commit())
    {
      Serial.println("ERROR! Data commit failed");
    }
    sprintf(filename, "/sdcard/%s/%d-%05d.jpg", folder, randomNumber, fileNumber);
  }
  else
  {
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%F_%H_%M_%S", &timeinfo);
    sprintf(filename, "/sdcard/%s/%s.jpg", folder, strftime_buf);
  }
  savePhotoToSDCard(filename, fb);
  free(filename);
}

void deepSleep()
{
  Serial.println("Deep sleep for " + String(sleepTime) + " seconds");
  Serial.flush();
  int sleep = sleepTime;
  if (isNight)
  {
    sleep = sleepTimeNight;
  }
  esp_sleep_enable_timer_wakeup(sleep * 1000000);
  esp_deep_sleep_start();
}

void ota()
{
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER)
  {
    return;
  }
  Serial.println("Waiting for update from Arduino editor.");
  ArduinoOTA.setHostname(folder);
  ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";

        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("Start updating " + type);
      })
      .onEnd([]() {
        Serial.println("\nEnd");
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
          Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
          Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
          Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
          Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
          Serial.println("End Failed");
      });

  ArduinoOTA.begin();
  unsigned long start = millis();
  while (millis() - start <= otaWaitingTime * 1000)
  {
    ArduinoOTA.handle();
  }
}

camera_fb_t *getPicture()
{
  esp_err_t err = initCamera();
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    deepSleep();
  }
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed, check if camera is connected");
    deepSleep();
  }
  return fb;
}