#include <M5Unified.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include <esp_netif.h>

#include "wifi_manager.h"

#define BLINK_GPIO CONFIG_BLINK_GPIO

static led_strip_handle_t led_strip;

static const char *TAG = "example";

void cb_connection_ok(void *pvParameter)
{
  ESP_LOGI(TAG, "I have a connection!");

  ip_event_got_ip_t *param = (ip_event_got_ip_t *)pvParameter;

  /* transform IP to human readable string */
  char str_ip[16];
  esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);

  ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);
}


void setup(void)
{




  /* start the wifi manager */
  wifi_manager_start();

      // clears memory
  wifi_manager_save_sta_config();


  wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);

  auto cfg = M5.config(); // Assign a structure for initializing M5Stack
  // If config is to be set, set it here
  // Example.
  // cfg.external_spk = true;

  M5.begin(cfg); // initialize M5 device

  ESP_LOGI(TAG, "Example configured to blink addressable LED!");
  /* LED strip initialization with the GPIO and pixels number*/
  led_strip_config_t strip_config = {
      .strip_gpio_num = BLINK_GPIO,
      .max_leds = 1, // at least one LED on board
      .led_pixel_format = LED_PIXEL_FORMAT_GRB,
      .led_model = LED_MODEL_SK6812,

  };
  led_strip_rmt_config_t rmt_config = {
      .resolution_hz = 10 * 1000 * 1000, // 10MHz
  };
  ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
  /* Set all LED off to clear all pixels */
  led_strip_clear(led_strip);
}

void blink_led(int on)
{
  /* If the addressable LED is enabled */
  if (on)
  {
    /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
    led_strip_set_pixel(led_strip, 0, 255, 255, 255);
    /* Refresh the strip to send data */
    led_strip_refresh(led_strip);
  }
  else
  {
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
  }
}

void loop(void)
{
  M5.delay(50);

  M5.update();

  static constexpr const char *const names[] = {"none", "wasHold", "wasClicked", "wasPressed", "wasReleased", "wasDeciedCount"};

  /// BtnA,BtnB,BtnC,BtnEXT: "isPressed"/"wasPressed"/"isReleased"/"wasReleased"/"wasClicked"/"wasHold"/"isHolding"  can be use.
  int state = M5.BtnA.wasHold()               ? 1
              : M5.BtnA.wasClicked()          ? 2
              : M5.BtnA.wasPressed()          ? 3
              : M5.BtnA.wasReleased()         ? 4
              : M5.BtnA.wasDecideClickCount() ? 5
                                              : 0;
  if (state)
  {
    M5_LOGI("BtnA:%s  count:%d", names[state], M5.BtnA.getClickCount());
  }

  blink_led(state);
}



extern "C"
{
  void loopTask(void *)
  {
    setup();
    while (true)
    {
      loop();
    }
    vTaskDelete(NULL);
  }

  void app_main()
  {
    xTaskCreatePinnedToCore(loopTask, "loopTask", 8192, NULL, 1, NULL, 1);
  }
}
