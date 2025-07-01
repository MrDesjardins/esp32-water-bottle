const int sensorPin = 34;  // Analog pin connected to voltage divider
const float vcc = 3.3;     // ESP32 operating voltage
const int adcMax = 4095;   // 12-bit ADC on ESP32

// Calibration parameters (adjust after testing)
const float minPressure = 0.0;     // Analog value when no weight
const float maxPressure = 3000.0;  // Analog value under known max weight
const float maxWeight = 1000.0;    // Max weight in grams corresponding to maxPressure

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // Set resolution to 12 bits (0–4095)
}

void loop() {
  int analogValue = analogRead(sensorPin);

  // Convert to voltage if needed
  float voltage = (analogValue / float(adcMax)) * vcc;

  // Simple linear mapping from pressure to weight (requires calibration!)
  float weight = mapWeight(analogValue);

  Serial.print("Analog Value: ");
  Serial.print(analogValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 3);
  Serial.print(" V | Approx. Weight: ");
  Serial.print(weight, 1);
  Serial.println(" g");

  delay(500);
}

float mapWeight(int analogValue) {
  if (analogValue < minPressure) return 0;
  if (analogValue > maxPressure) return maxWeight;

  // Linear interpolation
  return (analogValue - minPressure) * (maxWeight / (maxPressure - minPressure));
}
