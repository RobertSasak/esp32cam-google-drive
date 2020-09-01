#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"
#include "esp_camera.h"

const int sleepTime = 300; // in seconds
const char *ssid = "your wifi network";
const char *password = "your wifi password";
const char *folder = "ESP32";
const char *myDomain = "script.google.com";
const char *scriptID = "here comes very long script ID that you will copy from url of script.google.com";
const int port = 443;

int waitingTime = 10000; // Wait x seconds for response.

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

bool connectWifi()
{
  WiFi.mode(WIFI_STA);

  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
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

  if (!connectWifi())
  {
    Serial.println("Cannot connect to Wifi");
    deepSleep();
  }

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
  config.frame_size = FRAMESIZE_UXGA; // UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
  config.jpeg_quality = 3;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    deepSleep();
  }
}

void loop()
{
  saveCapturedImage();
  deepSleep();
}

void saveCapturedImage()
{
  Serial.println("Connect to " + String(myDomain));
  WiFiClientSecure client;

  if (client.connect(myDomain, port))
  {
    Serial.println("Connection successful");

    camera_fb_t *fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb)
    {
      Serial.println("Camera capture failed");
      deepSleep();
    }
    String url = "/macros/s/" + String(scriptID) + "/exec?folder=" + String(folder);

    Serial.println("Sending image to Google Drive.");
    Serial.println("Size: " + String(fb->len) + "byte");

    client.println("POST " + url + " HTTP/1.1");
    client.println("Host: " + String(myDomain));
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

    esp_camera_fb_return(fb);

    Serial.println("Waiting for response.");
    long int StartTime = millis();
    while (!client.available())
    {
      Serial.print(".");
      delay(100);
      if ((StartTime + waitingTime) < millis())
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
    Serial.println("Connected to " + String(myDomain) + " failed.");
  }
  client.stop();
}

void deepSleep()
{
  Serial.println("Deep sleep for " + String(sleepTime) + " seconds");
  Serial.flush();
  esp_sleep_enable_timer_wakeup(sleepTime * 1000000);
  esp_deep_sleep_start();
}
