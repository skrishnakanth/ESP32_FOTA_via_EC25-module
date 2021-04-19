/*
THE PROJECT IS DONE WITH REFERENCE OF NATIVE OTA PROJECT OF ESP32 OPEN SOURCE CODE OFFICIALLY PUBLISHED BY ESPRESSIF.
REFERED PROJECT LINK: https://github.com/espressif/esp-idf/tree/master/examples/system/ota/native_ota_example
THIS PROJECT IS COMPLETELY AUTHORISED BY THE COMPANY BANYAN_IOT_SOLUTIONS AND FOLLOWING CERTAIN POLICIES WITH ANY FURTHER MODIFICATION.
**********
AUTHOR:S.KRISHNAKANTH
COMPANY:BANYAN_IOT_SOLUTIONS.
**********
*/

#include <string.h>
#include <Arduino.h>
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "errno.h"
#include "driver/gpio.h"
#define RXD2 16
#define TXD2 17

#define RXD3 9
#define TXD3 10
#define EC25 Serial2
#define ESP32 Serial
#define BUFFSIZE 4096


//variables declaration --BEGIN--
int ota_begin = 0;
int binary_file_length = 0;
int j = 0;
int i = 0;
bool ota_flash = false;
int l = 0;
bool end = false;
unsigned int file_Sector = 0, extra_sector = 0;
String read_file = "AT+QFREAD=3000,";
char sector_val[4] = {0};
unsigned int size_of_buff = 0, max_serial_val = 0;
static char ota_write_data[BUFFSIZE + 1] = {0};
String rec1 = "";
String rec2 = "";
int start = 0;
//variables declaration --END--

//Handler definition
esp_ota_handle_t update_handle = 0;
esp_err_t err;
//Handler definition

//function definition --BEGIN--
unsigned int EC25_response(unsigned int timeout);
void EC25_connection();
void EC25_ECHO_OFF();
void EC25_WEB_CONN();
void EC25_FILE_EXCHANGE();
static void ota_example_task();
void MC25_FREAD();
void MC25_FCLOSE();
void END();
//function definition --END--

void setup()
{
  ESP32.begin(115200);
  EC25.begin(115200, SERIAL_8N1, RXD2, TXD2);
  EC25_ECHO_OFF();
}

void loop()
{
  EC25_connection();
  EC25_WEB_CONN();
  EC25_FILE_EXCHANGE();
  MC25_FREAD();
  MC25_FCLOSE();
  END();
}

unsigned int EC25_response(unsigned int timeout)
{
  unsigned int y = 0, k = 0;
  unsigned long current_time = millis();
  char EC25_reply_char = {0};
  String num_string = "";
  char EC25_reply[5000] = {0};
  String temp_reply = "";
  unsigned int EC25_status[50] = {0};
  memset(EC25_reply, 0, sizeof EC25_reply);
  memset(EC25_status, 0, sizeof EC25_status);

  while ((millis() - current_time) < timeout)
  {

    if (EC25.available())
    {
      EC25_reply_char = EC25.read();
      EC25_reply[y] = EC25_reply_char;
      y++;
    }
    max_serial_val = y;
  }
  ESP32.print("EC25 response:");
  for (int a = 0; a < y; a++)
  {
    ESP32.print(EC25_reply[a]);
  }

  if (EC25_reply[2] == 'C' && EC25_reply[9] == ' ')
  {
    for (int j = 10; j < 20; j++)
    {
      if (isDigit(EC25_reply[j]))
      {
        num_string += EC25_reply[j];
      }
      else if (EC25_reply[j] == '\n')
      {
        EC25_status[k] = num_string.toInt();
        num_string = "";
        k++;
      }
      if (EC25_status[0] == BUFFSIZE || EC25_status[0] == extra_sector)
      {
        if (EC25_status[0] == extra_sector)
        {
          end = true;
        }
        for (int i = 0, j = 16; i < y && j < y; i++, j++)
        {
          if (j == max_serial_val - 6)
          {
            break;
          }
          ota_write_data[i] = EC25_reply[j];
        }
        ota_example_task();
        ESP32.print("val:");
        ESP32.println(EC25_status[0]);
        EC25_status[0] = 0;
      }
    }
  }
  if (EC25_reply[8] == '+' && EC25_reply[14] == 'G')
  {
    for (int j = 8; j < 38; j++)
    {

      if (isDigit(EC25_reply[j]))
      {
        num_string += EC25_reply[j];
      }
      else if (EC25_reply[j] == ',' || EC25_reply[j] == '\n')
      {
        EC25_status[k] = num_string.toInt();
        num_string = "";
        k++;
      }
    }
    ESP32.print("EC25 STATUS:");
    for (int i = 0; i < k; i++)
    {
      ESP32.println(EC25_status[i]);
    }

    file_Sector = EC25_status[2] / 4096;
    extra_sector = EC25_status[2] - (file_Sector * 4096);
    if (extra_sector == 0)
    {
      ESP32.print("FILE SECTOR:");
      ESP32.println(file_Sector);
    }
    else
    {
      ESP32.print("FILE SECTOR:");
      ESP32.println(file_Sector);
      ESP32.print("REMAINING DATA:");
      ESP32.println(extra_sector);
    }
  }
  return 1;
}

void EC25_connection()
{
  ESP32.println("AT");
  EC25.println("AT");
  EC25_response(500);
  delay(2000);
}

void EC25_ECHO_OFF()
{

  delay(2000);
  ESP32.println("ATE0");
  EC25.println("ATE0");
  EC25_response(1000);
  start = 1;
  delay(1000);
}

void EC25_WEB_CONN()
{
  ESP32.println("AT+QHTTPCFG=\"contextid\",1");
  EC25.println("AT+QHTTPCFG=\"contextid\",1");
  EC25_response(1000);
  delay(2000);
  ESP32.println("AT+QHTTPURL=41,120");
  EC25.println("AT+QHTTPURL=41,120");
  EC25_response(1000);
  delay(2000);
  ESP32.println("http://103.207.4.72:8050/files/blink1.bin");
  EC25.println("http://103.207.4.72:8050/files/blink1.bin");
  EC25_response(1000);
  delay(2000);
  ESP32.println("AT+QHTTPGET=80");
  EC25.println("AT+QHTTPGET=80");
  EC25_response(5000);
  delay(2000);
  ESP32.println("AT+QHTTPREADFILE=\"RAM:6.txt\",80");
  EC25.println("AT+QHTTPREADFILE=\"RAM:6.txt\",80");
  EC25_response(20000);
  delay(2000);
}

void END()
{
  while (1)
  {
  }
}

void EC25_FILE_EXCHANGE()
{
  ESP32.println("AT+QFLST=\"RAM:6.txt\"");
  EC25.println("AT+QFLST=\"RAM:6.txt\"");
  EC25_response(1000);
  delay(2000);
  ESP32.println("AT+QFOPEN=\"RAM:6.txt\",0");
  EC25.println("AT+QFOPEN=\"RAM:6.txt\",0");
  EC25_response(1000);
  delay(2000);
  ESP32.println("AT+QFSEEK=3000,0,0");
  EC25.println("AT+QFSEEK=3000,0,0");
  EC25_response(1000);
  delay(2000);
}

void MC25_FREAD()
{
  for (int j = 0; j <= file_Sector; j++)
  {

    sprintf(sector_val, "%d", 4096);
    if (j == (file_Sector))
    {
      sprintf(sector_val, "%d", extra_sector);
    }
    EC25.print("AT+QFREAD=3000,");
    EC25.println(sector_val);
    EC25_response(500);
  }
}

void MC25_FCLOSE()
{
  ESP32.println("AT+QFCLOSE=3000");
  EC25.println("AT+QFCLOSE=3000");
  EC25_response(1000);
  delay(2000);
}

static void ota_example_task()
{
  const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

  if (ota_begin == 0)
  {
    ESP32.print("<-------------OTA BEGINS---------------->");
    const esp_partition_t *configured = esp_ota_get_boot_partition(); // 1
    const esp_partition_t *running = esp_ota_get_running_partition(); //2

    //OTA BEGIN
    err = esp_ota_begin(update_partition, update_partition->size, &update_handle);
    ESP32.print("esp_ota_begin err_state:");
    Serial.println(err);
    ota_begin = 1;
  }

    //OTA WRITE
  err = esp_ota_write(update_handle, (const void *)ota_write_data, BUFFSIZE);
  if (err != ESP_OK)
  {
    ESP32.print("ERROR in WRITING:");
    ESP32.println(err);
  }

  if (end == true)
  {
    Serial.print("remaining length val:");
    Serial.println(i);
    //OTA END
    err = esp_ota_end(update_handle);
    if (err != ESP_OK)
    {
      if (err == ESP_ERR_OTA_VALIDATE_FAILED)
      {
        Serial.println("Image validation failed, image is corrupted");
      }
    }

    //SHIFT BOOT PARTITION
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK)
    {
      Serial.print("esp_ota_set_boot_partition failed ");
      Serial.println(esp_err_to_name(err));
    }
    Serial.print("boot partition is set");
    esp_restart();
  }
}