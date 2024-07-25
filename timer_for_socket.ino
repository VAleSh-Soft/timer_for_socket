/**
 * @file timer_for_socket.ino
 * @author Vladimir Shatalov (valesh-soft@yandex.ru)
 *
 * @brief Реле времени на 30 минут с возможностью регулировки интервала
 *        срабатывания через UART;
 * 
 *        Команды управления реле через UART см. ниже;
 * 
 *        Включение модуля с зажатой кнопкой управления сбрасывает настройку
 *        интервала срабатывания реле к значению по умолчанию;
 *
 * @version 1.0
 * @date 25.07.2024
 *
 * @copyright Copyright (c) 2024
 */

#include <EEPROM.h>
#include <shButton.h>
#include <shTaskManager.h>

// ===================================================

#define UART_ON 1 // задействование интерфейса UART; 1 - включение, 0 - отключено

/* попутно с отладочным выводом задействуется настройка интервала срабатывания
 * реле через UART:
 * - команда 'w120' задает новый интервал; 120 - количество секунд;
 *     настройка сохраняется в EEPROM, максимальное значение - 86400 секунд (одни
 *     сутки, ограничение просто чтобы было);
 * - команда 'r' выводит состояние задачи:
 *   - заданный интервал срабатывания;
 *   - активна задача или нет;
 *   - оставшееся время работы задачи;
 * - команда 's' переключает состояние реле;
 */

// ==== настройки ====================================

constexpr uint8_t RELAY_PIN = 3;      // пин реле
constexpr uint8_t LED_GREEN_PIN = 6;  // пин зеленого светодиода
constexpr uint8_t LED_RED_PIN = 5;    // пин красного светодиода
constexpr uint8_t BTN_PIN = 11;       // пин кнопки
constexpr uint8_t RELAY_LEVEL = HIGH; // управляющий уровень реле

constexpr uint8_t MIN_LEVEL_FOR_LED = 0;   // минимальное значение ШИМ для зеленого светодиода
constexpr uint8_t MAX_LEVEL_FOR_LED = 250; // минимальное значение ШИМ для зеленого светодиода
constexpr uint8_t STEP_FOR_PWM = 5;        // шаг изменения ШИМ для зеленого светодиода

constexpr unsigned long MAX_TIMEOUT = 86400; // максимальное значение таймера

unsigned long relay_timeout = 1800000; // время выдержки в милисекундах по умолчанию

#define EEPROM_TIMEOUT_INDEX 50 // индекс в EEPROM для хранения интервала реле (uint32_t)

// ===================================================

#if UART_ON
#define PRINTLN(x) Serial.println(x)
#define PRINT(x) Serial.print(x)
#define WRITETIME(x) writeTime(x)
#else
#define PRINTLN(x)
#define PRINT(x)
#define WRITETIME(x)
#endif

shHandle leds_guard;  // задача управления светодиодами
shHandle relay_guard; // задача управления реле

shTaskManager tasks(2); // список задач

shButton btn(BTN_PIN); // управляющая кнопка

#if UART_ON
// вывод в Serial значения времени в формате hh:mm:ss
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

// установка нового интервала реле
void setTimeout(uint32_t _time)
{
  PRINTLN();
  PRINTLN(F("Setting up a new relay timeout"));

  relay_timeout = _time * 1000;
  tasks.setTaskInterval(relay_guard, relay_timeout, false);
  EEPROM.put(EEPROM_TIMEOUT_INDEX, _time);

  PRINT(F("New relay timeout: "));
  PRINT(_time);
  PRINT(F(" sec ("));
  writeTime(relay_timeout, false);
  PRINTLN(F(")"));
}

// вывод в сериал состояния реле
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

// очистка буфера Serial
void clearSerial()
{
  while (Serial.available())
  {
    Serial.read();
    delay(1);
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
    tasks.startTask(relay_guard);
    digitalWrite(RELAY_PIN, RELAY_LEVEL);
    timer = millis();
    PRINTLN(F("Task of relay started"));
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
    EEPROM.put(EEPROM_TIMEOUT_INDEX, relay_timeout / 1000);
  }
  else
  {
    relay_timeout = _time * 1000;
  }

  relay_guard = tasks.addTask(relay_timeout, setRelay, false);
  leds_guard = tasks.addTask(50ul, setLeds);

  btn.setLongClickMode(LCM_ONLYONCE);
  btn.setTimeoutOfLongClick(1000ul);

  PRINTLN(F("Device is started"));
}

void loop()
{
  tasks.tick();

  // запускаем и останавливаем задачу удержанием нажатой кнопки не менее 1 секунды
  if (btn.getButtonState() == BTN_LONGCLICK)
  {
    PRINTLN(F("Button is pressed"));
    setRelay();
  }

#if UART_ON
  // обработка ввода/вывода в UART
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
#endif
}