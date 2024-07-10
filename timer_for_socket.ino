/**
 * @brief Реле времени на 30 минут с возможностью регулировки интервала
 *        срабатывания через UART
 *
 */

#include <EEPROM.h>
#include <shButton.h>
#include <shTaskManager.h>

// ===================================================

#define UART_ON 1 // задействование интерфейса UART; 1 - включение, 0 - отключено

/* попутно с отладочным выводом задействуется настройка интервала срабатывания
 * реле через UART:
 * - команда 'w120' задает новый интервал; 120 - количество секунд;
 *     настройка сохраняется в EEPROM, максимальное значение - 86400 (одни
 *     сутки, ограничение просто чтобы было);
 * - команда 'r' выводит состояние задачи:
 *   - заданный интервал срабатывания;
 *   - активна задача или нет;
 *   - оставшееся время работы задачи;
 * - команда 's' переключает состояние реле;
 */

// ==== настройки ====================================

constexpr uint8_t relay_pin = 3;      // пин реле
constexpr uint8_t led_green_pin = 6;  // пин зеленого светодиода
constexpr uint8_t led_red_pin = 5;    // пин красного светодиода
constexpr uint8_t btn_pin = 11;       // пин кнопки
constexpr uint8_t relay_level = HIGH; // управляющий уровень реле

uint32_t relay_timeout = 1800000; // время выдержки в милисекундах по умолчанию

#define EEPROM_TIMEOUT_INDEX 50 // индекс в EEPROM для хранения интервала реле (uint32_t)

// ===================================================

#if UART_ON
#define PRINT(x)          \
  Serial.print(millis()); \
  Serial.print(" - ");    \
  Serial.println(x)
#else
#define PRINT(x)
#endif

shHandle leds_guard;  // задача управления светодиодами
shHandle relay_guard; // задача управления реле

shTaskManager tasks(2); // список задач

shButton btn(btn_pin); // управляющая кнопка

#if UART_ON
// установка нового интервала реле
void setTimeout(uint32_t _time)
{
  Serial.println();
  Serial.println(F("Setting up a new relay timeout"));

  relay_timeout = _time * 1000;
  tasks.setTaskInterval(relay_guard, relay_timeout, false);
  EEPROM.put(EEPROM_TIMEOUT_INDEX, _time);

  Serial.print(F("New relay timeout, sec: "));
  Serial.println(_time);
}

// вывод в сериал состояния реле
void getTaskOfRelayState()
{
  Serial.println();
  Serial.println(F("Status of task of relay:"));
  Serial.print(F("- timeout, sec: "));
  Serial.println(relay_timeout / 1000);
  Serial.print(F("- relay state: "));
  if (tasks.getTaskState(relay_guard))
  {
    Serial.println(F("ON"));
    Serial.print(F("- time left, sec: "));
    Serial.println(tasks.getNextTaskPoint(relay_guard) / 1000);
  }
  else
  {
    Serial.println(F("OFF"));
  }
  Serial.println();
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
  static uint8_t num = 0;
  static bool to_up = true;

  bool x = (digitalRead(relay_pin) == relay_level);
  if (x)
  {
    // зеленый светодиод плавно разгорается и плавно гаснет
    num += (to_up) ? 5 : -5;
    analogWrite(led_green_pin, num);
    to_up = (num == 250) ? false
                         : ((num == 0) ? true : to_up);
  }
  else
  {
    digitalWrite(led_green_pin, LOW);
    num = 0;
    to_up = true;
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
    digitalWrite(relay_pin, relay_level);
    PRINT(F("Task of relay started"));
  }
  else
  {
    tasks.stopTask(relay_guard);
    digitalWrite(relay_pin, !relay_level);
    PRINT(F("Task of relay stoped"));
  }
}

void setup()
{
#if UART_ON
  Serial.begin(115200);
#endif

  pinMode(relay_pin, OUTPUT);
  pinMode(led_green_pin, OUTPUT);
  pinMode(led_red_pin, OUTPUT);

  uint32_t _time;
  EEPROM.get(EEPROM_TIMEOUT_INDEX, _time);
  // если в EEPROM заданы некорректные данные или при включении была зажата
  // кнопка, задать интервал по умолчанию - 30 минут
  if (_time > 84600 || _time == 0 || !digitalRead(btn_pin))
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

  PRINT(F("Device is started"));
}

void loop()
{
  tasks.tick();

  // запускаем и останавливаем задачу удержанием нажатой кнопки не менее 1 секунды
  if (btn.getButtonState() == BTN_LONGCLICK)
  {
    PRINT(F("Button is hold"));
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
    // если это 'r' и в посылке больше ничего нету, выводим данные по задаче
    if (_command == 'r' && Serial.peek() < 0)
    {
      getTaskOfRelayState();
    }
    // иначе, если это символ 'w', считываем число, которое идет после него
    else if (_command == 'w')
    {
      uint32_t _time = (uint32_t)Serial.parseInt();
      if (_time > 86400)
      {
        _time = 86400;
      }

      clearSerial();
      setTimeout(_time);
    }
    else if (_command == 's')
    {
      setRelay();
    }
    else
    {
      clearSerial();
      Serial.println(F("Unknown command"));
    }
  }
#endif
}