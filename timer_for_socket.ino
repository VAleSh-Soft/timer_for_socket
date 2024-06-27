/**
 * @brief Тупое реле времени на 30 минут ))
 *
 */
constexpr uint8_t relay_pin = 3;          // пин реле
constexpr uint8_t led_green_pin = 6;      // пин зеленого светодиода
constexpr uint8_t led_red_pin = 5;        // пин красного светодиода
constexpr uint32_t delay_time = 1800000u; // задержка в милисекундах

void setup()
{
  pinMode(relay_pin, OUTPUT);
  pinMode(led_green_pin, OUTPUT);
  pinMode(led_red_pin, OUTPUT);

  digitalWrite(led_green_pin, HIGH);

  digitalWrite(relay_pin, HIGH);
  delay(delay_time);
  digitalWrite(relay_pin, LOW);

  digitalWrite(led_green_pin, LOW);
  digitalWrite(led_red_pin, HIGH);
}

void loop() {}