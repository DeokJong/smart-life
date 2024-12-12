void setup() {
  Serial.begin(9600, SERIAL_8N1, 14, 15);
}

void loop() {
  Serial.println("hi arduino");
  delay(1000);
}
