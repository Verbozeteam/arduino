
const int LIGHT_THRESHOLD = 400; // if light ([0-1023]) is less than this, it is considered "dark"
const unsigned long LIGHT_TURN_ON_TIME_MS = 15000; // when movement happens, light turns on for this long, until reactivation is required
const int LIGHT_CHANGE_STEP = 2; // step to increase or decrease light output brightness
const int force_jumper_1 = 1;
const int force_jumper_2 = 0;

const int MAX_NUM_SENSORS = 3;
const int MAX_NUM_PIR_SENSORS = MAX_NUM_SENSORS;
const int MAX_NUM_PHOTO_SENSORS = 1;

const int JUMPER_1_PIN = 2;
const int JUMPER_2_PIN = 12;
const int LIGHT_SENSOR_PIN_START = A3;
const int MOTION_SENSOR_PIN_START = A4;
const int INDICATION_OUTPUT_PIN_START = 3;

struct {
  unsigned long start_time_ms;
  int cur_output;
} g_lightProps;

int countNumSensors() {
  return 3 - ((digitalRead(JUMPER_1_PIN) == HIGH && !force_jumper_1) ? 1 : 0) - ((digitalRead(JUMPER_2_PIN) == HIGH && !force_jumper_2) ? 1 : 0);
}

bool isDark(int numSensors) {
  // Take the average reading of all sensors
  int averageReading = 0;
  for (int i = 0; i < numSensors; i++)
    averageReading += analogRead(LIGHT_SENSOR_PIN_START+i);
  averageReading /= numSensors;

  return averageReading < LIGHT_THRESHOLD;
}

bool isMotion(int numSensors) {
  // any motion means there is motion
  for (int i = 0; i < numSensors; i++)
    if (digitalRead(MOTION_SENSOR_PIN_START+i) == HIGH)
      return true;
  return false;
}

void setup() {
  g_lightProps.cur_output = 0;
  g_lightProps.start_time_ms = -LIGHT_TURN_ON_TIME_MS - 1;
  
  pinMode(JUMPER_1_PIN, INPUT_PULLUP);
  pinMode(JUMPER_2_PIN, INPUT_PULLUP);

  for (int i = 0; i < MAX_NUM_SENSORS; i++) {
    pinMode(LIGHT_SENSOR_PIN_START+i, INPUT);
    pinMode(MOTION_SENSOR_PIN_START+i, INPUT);
    pinMode(INDICATION_OUTPUT_PIN_START, OUTPUT);
  }

  Serial.begin(9600);
}

void loop() {
  unsigned long cur_time_ms = millis();
  int numSensors = countNumSensors();
  bool dark = isDark(min(numSensors, MAX_NUM_PHOTO_SENSORS));
  bool motion = isMotion(min(numSensors, MAX_NUM_PIR_SENSORS));
  int prev_light_value = g_lightProps.cur_output;

  if (dark && motion)
    g_lightProps.start_time_ms = cur_time_ms;
  
  if (cur_time_ms - g_lightProps.start_time_ms > LIGHT_TURN_ON_TIME_MS ||               // if light has started more than LIGHT_TURN_ON_TIME_MS ago
      g_lightProps.start_time_ms+LIGHT_TURN_ON_TIME_MS < g_lightProps.start_time_ms) {  // or if time overflowed
    // then turn off the light
    g_lightProps.cur_output = max(0, g_lightProps.cur_output - LIGHT_CHANGE_STEP);
  } else {
    // then turn on the light
    g_lightProps.cur_output = min(255, g_lightProps.cur_output + LIGHT_CHANGE_STEP);
   
  }

  if (prev_light_value != g_lightProps.cur_output)
    for (int i = 0; i < numSensors; i++)
      analogWrite(INDICATION_OUTPUT_PIN_START+i, g_lightProps.cur_output);

  delay(10);
}









