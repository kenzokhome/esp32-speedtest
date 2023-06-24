class ledDevice {
public:
  int pin;
  unsigned long speed;
  int freq;
  int ledChannel;
  int resolution;
public:
  ledDevice(int pinNo, unsigned long speedNo) {
    pin = pinNo;
    speed = speedNo;
    pinMode(pinNo, OUTPUT);
    digitalWrite(pinNo, LOW);
  }

  ledDevice(int pinNo, unsigned long speedNo, int ledChannelNo) {
    pin = pinNo;
    speed = speedNo;
    freq = 5000;
    ledChannel = ledChannelNo;
    resolution = 8;
    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(pinNo, ledChannel);
    ledcWrite(ledChannel, 0);
  }
  void TurnLEDOn(int pwm = 10) {
    ledcWrite(ledChannel, pwm);
  }

  void TurnLEDOff() {
    ledcWrite(ledChannel, 0);
  }
};
