//
// mqtt-onair-indicator-for-wioterminal.ino
//
// URL: https://github.com/yoggy/mqtt-onair-indicator-for-wioterminal
//
#include"TFT_eSPI.h"
#include"Free_Fonts.h"                 // https://github.com/Seeed-Studio/Seeed_Arduino_LCD/blob/master/examples/320%20x%20240/All_Free_Fonts_Demo/Free_Fonts.h
#include "lcd_backlight.hpp"          // https://github.com/Seeed-Studio/Seeed_Arduino_Sketchbook/blob/master/examples/WioTerminal_BackLight/lcd_backlight.hpp
#include "rpcWiFi.h"                      // https://wiki.seeedstudio.com/Wio-Terminal-Network-Overview/
#include <PubSubClient.h>         // https://github.com/knolleary/pubsubclient

#include "config.h"

WiFiClient wifi_client;
PubSubClient mqtt_client(mqtt_host, mqtt_port, nullptr, wifi_client);

TFT_eSPI tft;
static LCDBackLight backLight;

bool is_on_air = false;

void setup() {
  Serial.begin(115200);

  // TFT settings
  tft.begin();
  tft.setRotation(3);
  tft.setFreeFont(FSSBO24);

  // backlight settings
  digitalWrite(LCD_BACKLIGHT, HIGH);
  backLight.initialize();

  // 5 way switch settings https://wiki.seeedstudio.com/Wio-Terminal-Switch/
  pinMode(WIO_5S_UP, INPUT_PULLUP);
  pinMode(WIO_5S_DOWN, INPUT_PULLUP);
  pinMode(WIO_5S_LEFT, INPUT_PULLUP);
  pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
  pinMode(WIO_5S_PRESS, INPUT_PULLUP);

  drawConnecting();

  Serial.println("WiFi connecting...");

  // Wifi
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    count ++;
    if (count > 20) reboot();
  }
  Serial.println("WiFi connected!");

  // MQTT
  bool rv = false;
  if (mqtt_use_auth == true) {
    rv = mqtt_client.connect(mqtt_client_id, mqtt_username, mqtt_password);
  }
  else {
    rv = mqtt_client.connect(mqtt_client_id);
  }
  if (rv == false) {
    Serial.println("mqtt connecting failed...");
    reboot();
  }
  Serial.println("MQTT connected!");

  mqtt_client.setCallback(recv_callback);
  mqtt_client.subscribe(mqtt_onair_data_topic);

  drawConnected();
}

int press_count = 0;

void loop() {
  // button
  if (digitalRead(WIO_5S_PRESS) == LOW) {
    if (press_count < 100) {
      press_count ++;
    }
    else if (press_count == 100) {
      toggleStatus();
      press_count ++; // max value is 101
    }
  }
  else {
    press_count = 0;
  }

  if (!mqtt_client.connected()) {
    reboot();
  }
  mqtt_client.loop();
}

void toggleStatus() {
  if (is_on_air == false) {
    // off_air -> on_air
    mqtt_client.publish(mqtt_onair_data_topic, "on_air", true);
  }
  else {
    // on_air -> off_air
    mqtt_client.publish(mqtt_onair_data_topic, "off_air", true);
  }
}

void recv_callback(char* topic, byte* payload, unsigned int length) {
  char payload_str[16];
  memset(payload_str, 0, 16);
  for (int i = 0; i < (length < 15 ? length : 15); ++i) {
    payload_str[i] = payload[i];
  }
  
  Serial.print("recv_callback : topic=");
  Serial.print(topic);
  Serial.print(", payload=");
  Serial.println((const char *)payload_str);

  if (strcmp((const char *)payload_str, "on_air") == 0) {
    is_on_air = true;
  }
  else {
    is_on_air = false;
  }
  redraw();
}

void redraw() {
  if (is_on_air) {
    drawOnAir();
  }
  else {
    drawOffAir();
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void reboot() {
  drawText("REBOOT!!", 75, 90, TFT_BLACK, TFT_YELLOW, 100);
  delay(1000);

  NVIC_SystemReset();  // for SAMD
  for (;;) {
    delay(1000);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void drawOnAir() {
  drawText("ON AIR", 75, 90, TFT_WHITE, TFT_RED, 100);
}

void drawOffAir()
{
  drawText("OFF AIR", 55, 90, TFT_DARKGREEN, TFT_BLACK, 6);
}

void drawConnecting()
{
  drawText("Connecting...", 10, 90, TFT_WHITE, TFT_BLUE, 10);
}

void drawConnected()
{
  drawText("Connected", 10, 90, TFT_WHITE, TFT_GREEN, 10);
}

void drawText(const char *str, int x, int y, int text_color, int background_color, int brightness)
{
  backLight.setBrightness(brightness);
  tft.fillScreen(background_color);

  tft.setTextColor(text_color, background_color);
  tft.drawString(str, x, y);
}
