/**
 * @file timer_for_socket.ino
 * @author Vladimir Shatalov (valesh-soft@yandex.ru)
 *
 * @brief Реле времени на 30 минут с возможностью регулировки интервала
 *        срабатывания через UART;
 *
 *        Команды управления реле через UART см. в файле header_file.h;
 *
 *        Включение модуля с зажатой кнопкой управления сбрасывает настройку
 *        интервала срабатывания реле к значению по умолчанию;
 *
 * @version 3.5.4
 * @date 22.06.2025
 *
 * @copyright Copyright (c) 2024
 */

#include <EEPROM.h>
#include <shButton.h>
#include <shTaskManager.h>
#include "header_file.h"

// ===================================================

#if UART_ON
void writeTime(unsigned long _time, bool line_break = true)
{
  _time /= 1000;
  PRINT(_time / 3600);
  PRINT(":");
  _time %= 3600;
  if (_time / 60 < 10)
  {
    PRINT(0);
  }
  PRINT(_time / 60);
  PRINT(F(":"));
  _time %= 60;
  if (_time < 10)
  {
    PRINT(0);
  }

  (line_break) ? PRINTLN(_time) : PRINT(_time);
}

void setTimeout(uint32_t _time)
{
  PRINTLN();
  PRINTLN(F("Setting up a new relay timeout"));

  relay_timeout = _time * 1000ul;
  tasks.setTaskInterval(relay_guard, relay_timeout, false);
  EEPROM.put(EEPROM_TIMEOUT_INDEX, _time);

  PRINT(F("New relay timeout: "));
  PRINT(_time);
  PRINT(F(" sec ("));
  writeTime(relay_timeout, false);
  PRINTLN(F(")"));
}

void getTaskOfRelayState()
{
  PRINTLN();
  PRINTLN(F("Status of task of relay:"));
  PRINT(F("- timeout: "));
  writeTime(relay_timeout);
  PRINT(F("- relay state: "));
  if (tasks.getTaskState(relay_guard))
  {
    PRINTLN(F("ON"));
    PRINT(F("- time left: "));
    writeTime(tasks.getNextTaskPoint(relay_guard));
  }
  else
  {
    PRINTLN(F("OFF"));
  }
  PRINTLN();
}

void clearSerial()
{
  while (Serial.available())
  {
    Serial.read();
    delay(1);
  }
}

void checkSerial()
{
  uint8_t n = Serial.available();
  if (n > 0)
  {
    delay(5);
    // считываем первый символ
    unsigned char _command = Serial.read();
    // если это 'r', и в посылке больше ничего нету, выводим данные по задаче
    if (_command == 'r' && Serial.peek() < 0)
    {
      getTaskOfRelayState();
    }
    // если это символ 'w', считываем число, которое идет после него
    else if (_command == 'w')
    {
      uint32_t _time = (uint32_t)Serial.parseInt();
      if (_time > MAX_TIMEOUT)
      {
        _time = MAX_TIMEOUT;
      }
      if (_time > 0)
      {
        setTimeout(_time);
      }
      else
      {
        PRINTLN(F("Incorrect number"));
      }
    }
    // если это 's', и в посылке больше ничего нет, переключаем состояние реле
    else if (_command == 's' && Serial.peek() < 0)
    {
      setRelay();
    }
    else
    {
      PRINTLN(F("Unknown command"));
    }
    clearSerial();
  }
}
#endif

void setLeds()
{
  static uint8_t num = MIN_LEVEL_FOR_LED;
  static bool to_up = true;

  bool x = (digitalRead(RELAY_PIN) == RELAY_LEVEL);
  if (x)
  {
    // зеленый светодиод плавно разгорается и плавно гаснет
    num += (to_up) ? STEP_FOR_PWM : STEP_FOR_PWM * -1;
    analogWrite(LED_GREEN_PIN, num);
    to_up = (num == MAX_LEVEL_FOR_LED) ? false
                                       : ((num == MIN_LEVEL_FOR_LED) ? true : to_up);
  }
  else
  {
    digitalWrite(LED_GREEN_PIN, LOW);
    num = MIN_LEVEL_FOR_LED;
    to_up = true;
  }

  // красный светодиод всегда горит ровно
  digitalWrite(LED_RED_PIN, !x);
}

void setRelay()
{
  static unsigned long timer = 0;
  // если задача еще не запущена, запускаем ее и включаем реле
  // иначе наоборот - останавливаем и отключаем
  if (!tasks.getTaskState(relay_guard))
  {
    tasks.setTaskInterval(relay_guard, relay_timeout);
    digitalWrite(RELAY_PIN, RELAY_LEVEL);
    timer = millis();
    PRINTLN(F("Task of relay started"));
    PRINT(F("Set working hours: "));
    WRITETIME(relay_timeout);
  }
  else
  {
    tasks.stopTask(relay_guard);
    digitalWrite(RELAY_PIN, !RELAY_LEVEL);
    PRINTLN(F("Task of relay stoped"));
    PRINT(F("Working hours: "));
    WRITETIME(millis() - timer);
  }
}

void checkButton()
{
  switch (btn.getButtonState())
  {
    // удержание кнопки нажатой более 1 секунды переключает состояние задачи
  case BTN_LONGCLICK:
    PRINTLN(F("Long button click was recorded"));
    setRelay();
    break;
    // короткий клик срабатывает только для запуска задачи
  case BTN_ONECLICK:
    if (!tasks.getTaskState(relay_guard))
    {
      PRINTLN(F("Short button click was recorded"));
      setRelay();
    }
    break;
  }
}

void setup()
{
#if UART_ON
  Serial.begin(115200);
#endif

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);

  uint32_t _time;
  EEPROM.get(EEPROM_TIMEOUT_INDEX, _time);
  // если в EEPROM заданы некорректные данные или при включении была зажата
  // кнопка, задать интервал по умолчанию - 30 минут
  if (_time > MAX_TIMEOUT || _time == 0 || !digitalRead(BTN_PIN))
  {
    _time = RELAY_TIMEOUT_DEFAULT;
    EEPROM.put(EEPROM_TIMEOUT_INDEX, _time);
  }

  relay_timeout = _time * 1000ul;

  relay_guard = tasks.addTask(relay_timeout, setRelay, false);
  leds_guard = tasks.addTask(50ul, setLeds);

  btn.setVirtualClickOn();
  btn.setLongClickMode(LCM_ONLYONCE);
  btn.setTimeoutOfLongClick(1000ul);

  PRINTLN(F("Device is started"));
}

void loop()
{
  tasks.tick();
  checkButton();

#if UART_ON
  checkSerial();
#endif
}