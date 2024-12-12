String a;
void setup() {
  Serial.begin(9600);
}
void loop() {
  a = "";
  while (Serial.available()) {
    char transmit = Serial.read();
    a.concat(transmit);
  }
  Serial.println("Serial: " + a);
  delay(1000);
}
