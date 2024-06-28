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
  static uint8_t num = 0;
  static bool to_up = true;

  bool x = digitalRead(relay_pin);
  if (x)
  {
    // зеленый светодиод плавно разгорается и плавно гаснет
    num += (to_up) ? 10 : -10;
    analogWrite(led_green_pin, num);
    to_up = (num == 250) ? false
                         : ((num == 0) ? true : to_up);
  }
  else
  {
    digitalWrite(led_green_pin, LOW);
    num = 0;
  }

  // красный светодиод всегда горит ровно
  digitalWrite(led_red_pin, !x);
}

void setRelay()
{
  // если задача еще не запущена, запускаем ее и включаем реле
  // иначе наоборот - останавливаем и отключаем
  if (!tasks.getTaskState(relay_guard))
  {
    tasks.startTask(relay_guard);
    digitalWrite(relay_pin, HIGH);
  }
  else
  {
    tasks.stopTask(relay_guard);
    digitalWrite(relay_pin, LOW);
  }
}

void setup()
{
  pinMode(relay_pin, OUTPUT);
  pinMode(led_green_pin, OUTPUT);
  pinMode(led_red_pin, OUTPUT);

  relay_guard = tasks.addTask(relay_timeout, setRelay, false);
  leds_guard = tasks.addTask(50ul, setLeds);

  btn.setLongClickMode(LCM_ONLYONCE);
  btn.setTimeoutOfLongClick(1000ul);
}

void loop()
{
  tasks.tick();

  // запускаем и останавливаем задачу удержанием нажатой кнопки не менее 1 секунды
  if (btn.getButtonState() == BTN_LONGCLICK)
  {
    setRelay();
  }
}