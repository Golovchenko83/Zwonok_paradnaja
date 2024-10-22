#include <ESP8266WiFi.h> //Библиотека для работы с WIFI
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h> // Библиотека для OTA-прошивки
#include <PubSubClient.h>
#include <TimerMs.h>
#include <Wire.h>
#include <ErriezDHT22.h>
WiFiClient espClient;
PubSubClient client(espClient);
DHT22 dht22 = DHT22(D7);
TimerMs OTA_Wifi(10, 1, 0);
TimerMs Dht_t(15000, 1, 0);
const char *name_client = "zwonok-paradnaja";      // Имя клиента и сетевого порта для ОТА
const char *mqtt_reset = "zwonok_paradnaja_reset"; // Имя топика для перезагрузки
const char *ssid = "Beeline";                      // Имя точки доступа WIFI
const char *password = "sl908908908908sl";         // пароль точки доступа WIFI
const char *mqtt_server = "192.168.1.221";
String s;
long timeM;
byte buff_clear;
static const uint8_t Pin_zum = 12;
static const uint8_t Pin_zum_d = 14;
int data = 0, dht_tik = 0;
float dht_raw = 0, dht_gr = 0, dht_sr = 0;

void publish_send(const char *top, float &ex_data) // Отправка Показаний с сенсоров
{
  char send_mqtt[10];
  dtostrf(ex_data, -2, 1, send_mqtt);
  client.publish(top, send_mqtt, 1);
}

void signal1()
{
  tone(Pin_zum, 440, 250);
  tone(Pin_zum_d, 640, 350);
  delay(250);
  tone(Pin_zum, 165, 250);
  tone(Pin_zum_d, 365, 350);
  delay(250);
  tone(Pin_zum, 440, 250);
  tone(Pin_zum_d, 640, 350);
  delay(250);
  delay(250);

  tone(Pin_zum, 440, 250);
  tone(Pin_zum_d, 640, 350);
  delay(250);
  tone(Pin_zum, 165, 250);
  tone(Pin_zum_d, 365, 350);
  delay(250);
  tone(Pin_zum, 440, 250);
  tone(Pin_zum_d, 640, 350);
  delay(250);
  delay(250);

  tone(Pin_zum, 330, 125);
  tone(Pin_zum_d, 530, 225);
  delay(125);
  tone(Pin_zum, 294, 125);
  tone(Pin_zum_d, 494, 225);
  delay(125);
  tone(Pin_zum, 262, 125);
  tone(Pin_zum_d, 462, 225);
  delay(124);
  tone(Pin_zum, 494, 125);
  tone(Pin_zum_d, 694, 225);
  delay(125);
  tone(Pin_zum, 440, 125);
  tone(Pin_zum_d, 640, 225);
  delay(125);
  tone(Pin_zum, 494, 125);
  tone(Pin_zum_d, 694, 225);
  delay(125);
  tone(Pin_zum, 262, 125);
  tone(Pin_zum_d, 462, 225);
  delay(125);
  tone(Pin_zum, 294, 125);
  tone(Pin_zum_d, 494, 225);
  delay(125);

  tone(Pin_zum, 330, 250);
  tone(Pin_zum_d, 530, 350);
  delay(250);
  tone(Pin_zum, 165, 250);
  tone(Pin_zum_d, 365, 350);
  delay(250);
  tone(Pin_zum, 440, 250);
  tone(Pin_zum_d, 640, 350);
}

void callback(char *topic, byte *payload, unsigned int length)
{

  s = ""; // очищаем перед получением новых данных

  for (unsigned int iz = 0; iz < length; iz++)
  {
    s = s + ((char)payload[iz]); // переводим данные в String
  }

  data = atoi(s.c_str()); // переводим данные в int

  if ((String(topic)) == "zwonok_paradnaja" && data == 1)
  {
    signal1();
    ESP.wdtFeed();
    signal1();
    client.publish("zwonok_paradnaja", "0", 1);
    data = 0;
  }
}

void wi_fi_con()
{
  WiFi.mode(WIFI_STA);
  WiFi.hostname(name_client); // Имя клиента в сети
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setHostname(name_client); // Задаем имя сетевого порта
  ArduinoOTA.begin();                  // Инициализируем OTA
}

void loop()
{
  ESP.wdtFeed();

  if (Dht_t.tick())
  {
    float temp_am_raw = 0;
    if (dht22.available())
    {
      dht_tik++;
      temp_am_raw = dht22.readTemperature();
      temp_am_raw = (temp_am_raw / 10)-0.2;
      dht_sr = dht_sr + temp_am_raw;
      publish_send("Zwonok_temperature", temp_am_raw);
    }
    if (dht_tik == 60)
    {
      dht_sr = dht_sr / dht_tik;
      publish_send("Zwonok_temperature_gr", dht_sr);
      dht_tik = 0;
      dht_sr = 0;
    }
  }

  if (OTA_Wifi.tick()) // Поддержание "WiFi" и "OTA"  и Пинок :) "watchdog" и подписка на "Топики Mqtt"
  {
    ArduinoOTA.handle();     // Всегда готовы к прошивке
    client.loop();           // Проверяем сообщения и поддерживаем соедениние
    if (!client.connected()) // Проверка на подключение к MQTT
    {
      while (!client.connected())
      {
        ESP.wdtFeed();                   // Пинок :) "watchdog"
        if (client.connect(name_client)) // имя на сервере mqtt
        {
          client.subscribe(mqtt_reset);         // подписались на топик
          client.subscribe("zwonok_paradnaja"); // подписались на топик
          client.subscribe(name_client);        // подписались на топик

          // Отправка IP в mqtt
          char IP_ch[20];
          String IP = (WiFi.localIP().toString().c_str());
          IP.toCharArray(IP_ch, 20);
          client.publish(name_client, IP_ch);
        }
        else
        {
          delay(5000);
        }
      }
    }
  }
}

void setup()
{
  wi_fi_con();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  ESP.wdtDisable(); // Активация watchdog
  dht22.begin();
}
