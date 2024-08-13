## timer_for_socket - простое реле времени с фиксированным инетрвалом срабатывания

Простое реле времени для управления, например, розеткой, в которую включено устройство, время работы которого должно быть ограничено. Например, фумигатор. Реле построено с использованием **Arduino Uno/nano/pro mini** на базе **ATmega168/328** и любого релейного модуля.

- [Управление](#управление)
  - [Управление через UART](#управление-через-uart)
- [Индикация](#индикация)
- [Настройка прошивки](#настройка-прошивки)
- [Использованные сторонние библиотеки](#использованные-сторонние-библиотеки)


### Управление

Реле управляется одной кнопкой. Включение/отключение реле выполняется удержанием кнопки нажатой в течение не менее 1 секунды.

#### Управление через UART

Реле времени может мониториться, управляться и настраиваться через **UART**. Чтобы задействовать эту возможность, нужно задать **1** в строке `#define UART_ON 1`

Доступны команды:
- проверка состояния реле:
  - r - выводит заданный интервал срабатывания, состояние выхода на реле (включен/выключен) и, если реле включено, оставшееся время до отключения;
- управление реле:
  - s - переключение состояния реле;
- настройка интервала срабатывания:
  - wXXX - где XXX - новый интервал срабатывания в секундах; например, команда `w120` установит интервал 120 секунд;

### Индикация

Для индикации состояния реле используются два светодиода (или двойной светодиод с общим катодом). При отключенном реле светится красный светодиод, при включенном реле - плавно "мигает" зеленый. 

### Настройка прошивки

Все настройки задаются в скетче в блоке **настройки**:
- настройки реле:
  - `RELAY_PIN` - пин для подключения реле;
  - `RELAY_LEVEL` - управляющий уровень реле;
- настройки светодиодов:
  - `LED_GREEN_PIN`, `LED_RED_PIN` - пины для подключения анодов индикаторных светодиодов;
  - параметры ШИМ для зеленого светодиода:
    - `MIN_LEVEL_FOR_LED`, `MAX_LEVEL_FOR_LED` - минимальное и максимальное значение ШИМ;
    - `STEP_FOR_PWM` - шаг изменения ШИМ для зеленого светодиода;
- кнопка:
  - `BTN_PIN` - пин для подключения кнопки
- тайминги:
  - `RELAY_TIMEOUT_DEFAULT` - время выдержки по умолчанию, секунд; по умолчанию задано 1800 секунд (30 минут);
  - `MAX_TIMEOUT` - максимальное значение таймера, секунд; ограничение, чтобы просто было )) ;
- EEPROM:
  - `EEPROM_TIMEOUT_INDEX` - индекс в EEPROM для хранения интервала реле (uint32_t);

### Использованные сторонние библиотеки

**shButton.h** - https://github.com/VAleSh-Soft/shButton<br>
**shTaskManager.h** - https://github.com/VAleSh-Soft/shTaskManager<br>

<hr>

Если возникнут вопросы, пишите на valesh-soft@yandex.ru 