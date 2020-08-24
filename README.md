# esp32cam-google-drive

Upload photos from ESP32 camera to Google Drive folder

## Problem

ESP32 is an awesome piece of hardware. It would be awesome to use it as a timelapse camera. Instead of saving files to the SD card, one might want to upload pictures. But where? Google drive is perfect for such a purpose.

Google scripts do not support direct uploading of binary files. Photo is split into chunks which are converted to base64 and uploaded as text.

## Two parts

Solution consists of two parts. Arduino code and Google script.

### ESP32 camera code

Open file _esp32cam-google-drive.ino_ in Arduino and edit your wifi network SSID name and password as well as _scriptId_ which you will get once you upload _esp32.gs_ file into https://script.google.com/ .

### Google script code

Create a new project at https://script.google.com/ and copy there content of _esp32.gs_. Publish your newly created script by Publish > Deploy as web app. You will need to give it permission as asked.

## Inspered by

This repo is build on the top of amazing work of https://github.com/gsampallo/esp32cam-gdrive . However gsampallo solution worked only with small photos.
