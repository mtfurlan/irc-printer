CUSTOM_LIBS = ../libs
ESP_ROOT=../esp8266/
SKETCH=../irc-printer/irc-printer.ino


# UPLOAD_PORT = /dev/ttyUSB0
BOARD = nodemcuv2
# #default is fine for now
# #FLASH_DEF =

include ../makeEspArduino/makeEspArduino.mk
