#pragma once
#include <ArduinoJson.h>
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>
#include <deque>
#if !defined(TEST_UART_EXTERNAL_HAL) && !defined(TEST_UART_UNSUPPORTED)
#define ESP32 1
#endif
#ifdef TEST_UART_EXTERNAL_HAL
#define HASP_UART_EXTERNAL_HAL 1
#endif
#define HASP_USE_CUSTOM 1
#define HASP_USE_UART_TRANSPORT 1
#define HASP_USE_LITTLEFS 1
#define HASP_USE_GPIO 1
#ifndef HASP_USE_MQTT
#define HASP_USE_MQTT 0
#endif
#define CONFIG_IDF_TARGET_ESP32S3 1
#define ARDUINO_USB_CDC_ON_BOOT 1
#define RX 44
#define TX 43
#define TOUCH_SDA 8
#define TOUCH_SCL 9
#define TFT_BCKL 2
#define TAG_CONF 0
#define TAG_MSGR 1
#define F(x) x
#define LOG_ERROR(...) ((void)0)
#define LOG_INFO(...) ((void)0)
#include "my_custom.h"
extern std::vector<std::string> commands;
void dispatch_text_line(const char*, uint8_t);
