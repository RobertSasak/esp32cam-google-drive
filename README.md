![alt text](esp32cam-google-drive.png 'ESP32 cam + Google Drive')

# esp32cam-google-drive

Upload photos from ESP32 camera to Google Drive folder

## Problem

ESP32 is an awesome piece of hardware. It would be awesome to use it as a timelapse camera. Instead of saving files to the SD card, one might want to upload pictures. But where? Google drive is perfect for such a purpose.

Google scripts do not support direct uploading of binary files. Photo is split into chunks which are converted to base64 and uploaded as text.

## Two parts

Solution consists of two parts. Arduino code and Google script.

### ESP32 camera code

Open file _config.h_ in Arduino and edit your wifi network SSID name and password as well as _scriptId_ which you will get once you upload _esp32.gs_ file into https://script.google.com/ .

### Google script code

Create a new project at https://script.google.com/ and copy there content of _esp32.gs_. Publish your newly created script by Publish > Deploy as web app. You will need to give it permission as asked.

## Inspered by

This repo is build on the top of amazing work of https://github.com/gsampallo/esp32cam-gdrive . However gsampallo solution worked only with small photos.

## OTA

Over the air update was added for convenience. _AI-Thinker ESP32-CAM_ do not have option for choosing partition scheme in Arduino editor. By default it only offers _Huge App_. If you want to use OTA you need to add a custom _build.local.txt_ file into _Arduino15\packages\esp32\hardware\esp32\1.x.x\boards.txt_ as stated [here](https://arduino.stackexchange.com/questions/75198/why-doesnt-ota-work-with-the-ai-thinker-esp32-cam-board). Restart Arduino and select _Minimal SPIFFS_ partition scheme.

```
esp32cam.menu.PartitionScheme.huge_app=Huge APP (3MB No OTA/1MB SPIFFS)
esp32cam.menu.PartitionScheme.huge_app.build.partitions=huge_app
esp32cam.menu.PartitionScheme.huge_app.upload.maximum_size=3145728
esp32cam.menu.PartitionScheme.min_spiffs=Minimal SPIFFS (Large APPS with OTA)
esp32cam.menu.PartitionScheme.min_spiffs.build.partitions=min_spiffs
esp32cam.menu.PartitionScheme.min_spiffs.upload.maximum_size=1966080
```
