#define BLYNK_TEMPLATE_ID "TMPL6Ca7J5DUs"
#define BLYNK_TEMPLATE_NAME "Hien"
#define BLYNK_AUTH_TOKEN "2hirfTKyqp1-UEz9NeLKR7nGdnArtyrE"

#include <Wire.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <WiFiClient.h>

#define DHTPIN 5           // Pin kết nối cảm biến DHT
#define DHTTYPE DHT22      // Loại cảm biến DHT
#define FIRE_SENSOR_PIN 17 // Pin kết nối cảm biến lửa
#define BUZZER_PIN 33      // Pin còi báo động

// Khai báo đối tượng
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Địa chỉ I2C của LCD

// Thông tin WiFi và Blynk
char auth[] = "2hirfTKyqp1-UEz9NeLKR7nGdnArtyrE"; // Thay bằng Auth Token của bạn
char ssid[] = "HienNC";                        // Thay bằng SSID WiFi
char pass[] = "khendeptraidi";                    // Thay bằng mật khẩu WiFi

void taskReadSensors(void *pvParameters); 
void taskDisplayLCD(void *pvParameters); 
void taskBlynkUpdate(void *pvParameters);       // Thay bằng mật khẩu WiFi

// Biến dùng chung giữa các task
float temperature = 0;
float humidity = 0;
int fireDetected = HIGH; // Mặc định không phát hiện lửa

void setup() {
  Serial.begin(115200);

  // Khởi tạo Blynk và cảm biến
  Blynk.begin(auth, ssid, pass);
  dht.begin();

  // Khởi tạo LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("System Ready!");
  delay(2000);
  lcd.clear();

  // Cài đặt pin
  pinMode(FIRE_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Tạo các task với mức ưu tiên khác nhau
  xTaskCreate(taskReadSensors, "Read Sensors", 2048, NULL, 3, NULL); // Ưu tiên cao nhất
  xTaskCreate(taskDisplayLCD, "Display LCD", 2048, NULL, 2, NULL);   // Ưu tiên trung bình
  xTaskCreate(taskBlynkUpdate, "Blynk Update", 4096, NULL, 1, NULL); // Ưu tiên thấp nhất
}

void loop() {
  Blynk.run(); // Chạy Blynk trong loop chính
}

// Task 1: Đọc dữ liệu từ các cảm biến (Ưu tiên cao nhất)
void taskReadSensors(void *pvParameters) {
  while (true) {
    // Đọc nhiệt độ và độ ẩm
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    // Đọc trạng thái cảm biến lửa
    fireDetected = digitalRead(FIRE_SENSOR_PIN);

    // Kiểm tra lỗi cảm biến
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Lỗi đọc cảm biến!");
      temperature = 0;
      humidity = 0;
    }

    // Delay trước lần đọc tiếp theo
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// Task 2: Hiển thị thông số lên LCD và điều khiển còi báo động (Ưu tiên trung bình)
void taskDisplayLCD(void *pvParameters) {
  while (true) {
    if (fireDetected == HIGH) { // Không phát hiện lửa
      lcd.setCursor(0, 0);
      lcd.print("Temp:     ");
      lcd.print(temperature, 1);
      lcd.print("C");
      lcd.setCursor(0, 1);
      lcd.print("Humidity: ");
      lcd.print(humidity, 1);
      lcd.print("%");
      digitalWrite(BUZZER_PIN, LOW); // Tắt còi
    } else { // Phát hiện lửa
      lcd.setCursor(0, 0);
      lcd.print("FIRE ALERT!");
      lcd.setCursor(0, 1);
      lcd.print("T:");
      lcd.print(temperature, 1);
      lcd.print("C H:");
      lcd.print(humidity, 1);
      lcd.print("%");
      digitalWrite(BUZZER_PIN, HIGH); // Bật còi
    }

    // Delay trước khi cập nhật LCD tiếp theo
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// Task 3: Gửi thông tin lên Blynk và gửi thông báo khi phát hiện cháy (Ưu tiên thấp nhất)
void taskBlynkUpdate(void *pvParameters) {
  while (true) {
    // Gửi dữ liệu lên Blynk
    Blynk.virtualWrite(V0, temperature); // Gửi nhiệt độ
    Blynk.virtualWrite(V1, humidity);   // Gửi độ ẩm
    Blynk.virtualWrite(V4, !fireDetected); // Gửi trạng thái cảm biến lửa

    // Gửi thông báo khi phát hiện cháy
    if (fireDetected == LOW) { // Phát hiện lửa
      Blynk.logEvent("fire_alert", "Có Cháy");
    }

    // Delay trước lần gửi tiếp theo
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}