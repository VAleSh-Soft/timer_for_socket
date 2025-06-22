#pragma once

// ==== настройки ====================================

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

unsigned long RELAY_TIMEOUT_DEFAULT = 1800; // время выдержки по умолчанию, секунд

constexpr unsigned long MAX_TIMEOUT = 86400; // максимальное значение таймера, секунд

constexpr uint8_t RELAY_PIN = 3;      // пин реле
constexpr uint8_t RELAY_LEVEL = HIGH; // управляющий уровень реле

constexpr uint8_t LED_GREEN_PIN = 6; // пин зеленого светодиода
constexpr uint8_t LED_RED_PIN = 5;   // пин красного светодиода

constexpr uint8_t BTN_PIN = 11; // пин кнопки

constexpr uint8_t MIN_LEVEL_FOR_LED = 0;   // минимальное значение ШИМ для зеленого светодиода
constexpr uint8_t MAX_LEVEL_FOR_LED = 250; // минимальное значение ШИМ для зеленого светодиода
constexpr uint8_t STEP_FOR_PWM = 5;        // шаг изменения ШИМ для зеленого светодиода

constexpr uint16_t EEPROM_TIMEOUT_INDEX = 50; // индекс в EEPROM для хранения интервала реле (uint32_t)

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

// ===================================================

shHandle leds_guard;  // задача управления светодиодами
shHandle relay_guard; // задача управления реле

shTaskManager tasks(2); // список задач

shButton btn(BTN_PIN); // управляющая кнопка

unsigned long relay_timeout = RELAY_TIMEOUT_DEFAULT * 1000ul; // рабочий интервал

// ===================================================

#if UART_ON
// вывод в Serial значения времени в формате hh:mm:ss
void writeTime(unsigned long _time, bool line_break = true);
// установка нового интервала реле
void setTimeout(uint32_t _time);
// вывод в сериал состояния реле
void getTaskOfRelayState();
// очистка буфера Serial
void clearSerial();
// обработка ввода/вывода в UART
void checkSerial();
#endif

// управление светодиодами
void setLeds();
// управление реле
void setRelay();
// опрос кнопки
void checkButton();
