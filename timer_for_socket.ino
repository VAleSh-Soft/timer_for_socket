/**
 * @brief Реле времени на 30 минут без регулировки интервала
 *
 */

#include <shButton.h>
#include <shTaskManager.h>

constexpr uint8_t relay_pin = 3;     // пин реле
constexpr uint8_t led_green_pin = 6; // пин зеленого светодиода
constexpr uint8_t led_red_pin = 5;   // пин красного светодиода
constexpr uint8_t btn_pin = 11;      // пин кнопки

constexpr uint32_t relay_timeout = 1800000; // время выдержки в милисекундах

shHandle leds_guard;
shHandle relay_guard;

shTaskManager tasks(2);

shButton btn(btn_pin);

void setLeds()
{
  bool x = digitalRead(relay_pin);
  digitalWrite(led_green_pin, x);
  digitalWrite(led_red_pin, !x);
}

void setRelay()
{
  bool x = !digitalRead(relay_pin);
  digitalWrite(relay_pin, x);
  if (!x)
  {
    tasks.stopTask(relay_guard);
  }
}

void setup()
{
  pinMode(relay_pin, OUTPUT);
  pinMode(led_green_pin, OUTPUT);
  pinMode(led_red_pin, OUTPUT);

  relay_guard = tasks.addTask(relay_timeout, setRelay, false);
  leds_guard = tasks.addTask(100ul, setLeds);

  btn.setLongClickMode(LCM_ONLYONCE);
  btn.setTimeoutOfLongClick(1000ul);
}

void loop()
{
  tasks.tick();

  // запускаем и останавливаем задачу удержанием нажатой кнопки не менее 1 секунды
  if (btn.getButtonState() == BTN_LONGCLICK)
  {
    bool x = !tasks.getTaskState(relay_guard);
    tasks.setTaskState(relay_guard, x);
    tasks.taskExes(relay_guard);
  }
}