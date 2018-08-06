#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

#define TARGET_DEVICE_NAME      "QSTP Office Middleware"
#define WPA_SSID                "HB-DLINK"
#define WPA_PASS                "mody4326245"
#define AUTHENTICATION_TOKEN    "423fh424m234jf1j2hg2gh3k"
#define AUTHENTICATION_PASS     "ilovedogs<3"
#define AUTHENTICATION_MESSAGE  "{\"authentication\":{\"token\": \"" AUTHENTICATION_TOKEN "\",\"password\":\"" AUTHENTICATION_PASS "\",\"token_type\":4}}"

#define DEBUGGING
#ifdef DEBUGGING
#define LOG Serial.print
#define LOGPF Serial.printf
#define LOGLN Serial.println
#define LOGJSON(x) x.printTo(Serial)
#else
#define LOG(...)
#define LOGPF(...)
#define LOGLN(...)
#define LOGJSON(...)
#endif

#define THING_LIGHTSWITCH     0
#define THING_DIMMER          1
#define THING_PWM_DIMMER      2

#define THING_READERS_START   50
#define THING_DIGITAL_READER  50
#define THING_ANALOG_READER   51

struct THING_DEF {
  char type;
  const char* thing_id;
  int pin;
  int last_reading;
};

THING_DEF thing_defs[] = {
  {THING_LIGHTSWITCH, "lightswitch-1", 12, 0},
  {THING_LIGHTSWITCH, "lightswitch-2", 5, 0},
  {THING_DIGITAL_READER, "", 0, 0},
  {THING_DIGITAL_READER, "", 9, 0},
};
const int num_things = sizeof(thing_defs) / sizeof(THING_DEF);

WiFiClient insecureClient;
WiFiClientSecure secureClient;
WiFiClient* client = &insecureClient;

WiFiUDP Udp;
char target_middleware_ip[32] = {0};
int target_middleware_port = 0;
unsigned int udp_port = 7991;  // local port to listen on
char incoming_udp_packet[255];  // buffer for incoming packets

unsigned long current_time_ms = 0;
unsigned long delay_until_ms = 0;
unsigned long next_blink_ms = 0;
unsigned long blink_step_ms = 50;
int blink_pin_state = 0;
unsigned long next_heartbeat = 0;
unsigned long connection_timeout_time = 0;
unsigned long next_discovery_request_ms = 0;

void sendMessageToClient(const char* msg);
bool readClient();
void onConnected();
void performUDPScanning();
void updatePinReaders();
void onThingState(THING_DEF* thing, JsonObject& state);

void spinDelay(unsigned long t) {
  delay_until_ms = current_time_ms + t;
}

void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(LED_BUILTIN, OUTPUT);

  for (int i = 0; i < num_things; i++) {
    if (thing_defs[i].type >= THING_READERS_START)
      pinMode(thing_defs[i].pin, INPUT);
    else
      pinMode(thing_defs[i].pin, OUTPUT);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WPA_SSID, WPA_PASS);
  
  Udp.begin(udp_port);
}

void loop() {
  unsigned long _current_time_ms = millis();
  if (_current_time_ms < current_time_ms) {
    delay_until_ms = 0;
    next_blink_ms = 0;
    next_heartbeat = 0;
    connection_timeout_time = _current_time_ms + 6000;
    next_discovery_request_ms = 30000;
  }
  current_time_ms = _current_time_ms;

  if (current_time_ms >= next_blink_ms) {
    next_blink_ms = current_time_ms + blink_step_ms;
    blink_pin_state = 1 - blink_pin_state;
    digitalWrite(LED_BUILTIN, blink_pin_state);
  }

  performUDPScanning();

  // don't do any connection stuff if we are in a delayed state
  if (current_time_ms < delay_until_ms) {
    return;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    LOGLN("WiFi not connected...");
    blink_step_ms = 50;
    spinDelay(500);
    return;
  }
  
  if (!client->connected()) {
    blink_step_ms = 5000;
    if (strlen(target_middleware_ip) == 0 || target_middleware_port == 0)
      return;
    LOGPF("Attempting to connect to %s:%d (%s)...\n", target_middleware_ip, target_middleware_port, (client == &secureClient) ? "secure" : "insecure");
    if (!client->connect(target_middleware_ip, target_middleware_port)) {
      LOGLN(" Failed!");
      spinDelay(2000);
      return;
    }
    LOGLN(" Succeeded!");
    onConnected();
  }

  if (!readClient() || current_time_ms >= connection_timeout_time) {
    LOGLN("Failed to read from client");
    client->stop();
    spinDelay(2000);
    return;
  }
  
  updatePinReaders();
  
  // send heartbeat if time has come
  if (current_time_ms >= next_heartbeat) {
    next_heartbeat = current_time_ms + 5000;
    sendMessageToClient("{}");
  }

  if (!client->connected()) {
    LOGLN("Client connection died");
    spinDelay(2000);
    return;
  } else {
    blink_step_ms = 500;
  }
}

void sendMessageToClient(const char* msg) {
  char sizebuf[5];
  size_t msg_len = strlen(msg);
  sizebuf[0] = (msg_len & 0xFF);
  sizebuf[1] = ((msg_len >> 8) & 0xFF);
  sizebuf[2] = 0;
  sizebuf[3] = 0;
  client->write(sizebuf, 4);
  client->write(msg, msg_len);
}

void onConnected() {
  char buf[256];
  /**
   * Write authentication message
   */
  sendMessageToClient(AUTHENTICATION_MESSAGE);

  /**
   * Write code 2: subscribe as a listener only to things we care about to reduce overhead
   */
  strcpy(buf, "{\"code\":2,\"things\":[\"");
  for (int i = 0; i < num_things; i++) {
    if (i != 0)
      strcat(buf, ",\"");
    strcat(buf, thing_defs[i].thing_id);
    strcat(buf, "\"");
  }
  strcat(buf, "]}");
  sendMessageToClient(buf);
  
  connection_timeout_time = current_time_ms + 11000;
}

bool readClient() {
  int num_available = client->available();
  if (num_available > 4) {
    uint8_t msg_len_buf[4];
    client->peekBytes(msg_len_buf, 4);
    if (msg_len_buf[2] != 0 || msg_len_buf[3] != 0) {
      return false;
    }
    size_t msg_len = msg_len_buf[0] | (msg_len_buf[1] << 8);
    if (num_available >= 4 + msg_len) {
      client->read(msg_len_buf, 4); // consume the 4 bytes
      connection_timeout_time = current_time_ms + 11000;
      DynamicJsonBuffer json(msg_len);
      JsonObject& root = json.parseObject(*client);
      for (int i = 0; i < num_things; i++) {
        if (root.containsKey(thing_defs[i].thing_id)) {
          onThingState(&thing_defs[i], root[thing_defs[i].thing_id]);
        }
      }
    }
  }

  return true;
}

int parseDiscoveredServicePort(char* port_str) {
  if (!port_str) return 7990;
  for (int i = 0; i < strlen(port_str); i++)
    if (port_str[i] < '0' || port_str[i] > '9')
      return 7990;
  return atoi(port_str);
}

void performUDPScanning() {
  int packetSize = Udp.parsePacket();
  while (packetSize) {
    // LOGPF("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = Udp.read(incoming_udp_packet, 255);
    if (len >= 4) {
      if (incoming_udp_packet[0] == 0x29 && incoming_udp_packet[1] == 0xad && incoming_udp_packet[2] != 0) { // any packet that's not a discovery
        int datalen = incoming_udp_packet[3];
        if (len >= 4 + datalen && 4 + datalen < 255 && datalen > 0) {
          incoming_udp_packet[4+datalen] = 0;
          int service_type = incoming_udp_packet[2];
          char* device_name = incoming_udp_packet + 4;
          char* service_port = NULL;
          LOGPF("Device found: %s (%s, type=%d)\n", device_name, Udp.remoteIP().toString().c_str(), service_type);
          if (service_type == 3 || service_type == 8) {
            // a middleware
            for (int i = 0; i < datalen; i++) {
              if (device_name[i] == ':') {
                device_name[i] = 0;
                service_port = device_name + (i + 1);
                break;
              }
            }
            int service_port_int = parseDiscoveredServicePort(service_port);
            if (strcmp(device_name, TARGET_DEVICE_NAME) == 0 && strcmp(Udp.remoteIP().toString().c_str(), target_middleware_ip) != 0) {
              strcpy(target_middleware_ip, Udp.remoteIP().toString().c_str());
              target_middleware_port = service_port_int;
              LOGPF("Will try to connect to %s (%s:%d)\n", device_name, target_middleware_ip, target_middleware_port);
              if (client->connected())
                client->stop();
              if (service_type == 3)
                client = &insecureClient;
              else
                client = &secureClient;
            }
          }
        }
      }
    }
    
    packetSize = Udp.parsePacket();
  }
  
  if (current_time_ms >= next_discovery_request_ms && WiFi.status() == WL_CONNECTED) {
    next_discovery_request_ms = current_time_ms + 30000;
    IPAddress broadcastIp = WiFi.localIP();
    uint8_t packet[4] = {0x29, 0xad, 0, 0};
    broadcastIp[3] = 255;
    Udp.beginPacket(broadcastIp, udp_port);
    Udp.write(packet, 4);
    Udp.endPacket();
  }
}

void updatePinReaders() {
  char buf[256];
  for (int i = 0; i < num_things; i++) {
    if (thing_defs[i].type >= THING_READERS_START) {
      if (thing_defs[i].type == THING_DIGITAL_READER || thing_defs[i].type == THING_ANALOG_READER) {
        int reading = thing_defs[i].type == THING_DIGITAL_READER ? digitalRead(thing_defs[i].pin)
                                                                 : analogRead(thing_defs[i].pin);
        if (reading != thing_defs[i].last_reading) {
          thing_defs[i].last_reading = reading;
          sprintf(buf, "{\"port\": \"%c%d\", \"value\": %d}", thing_defs[i].type == THING_DIGITAL_READER ? 'd' : 'a', thing_defs[i].pin, reading);
          LOG("Sending ");
          LOGLN(buf);
          sendMessageToClient(buf);
        }
      }
    }
  }
}

void onThingState(THING_DEF* thing, JsonObject& state) {
  LOG("Got thing state ");
  LOG(thing->thing_id);
  LOG(": ");
  LOGJSON(state); LOGLN("");
  switch (thing->type) {
    case THING_LIGHTSWITCH: {
      if (!state.containsKey("intensity"))
        return;
      int intensity = state.get<int>("intensity");
      digitalWrite(thing->pin, intensity);
      break;
    }
    case THING_DIMMER: {
      break;
    }
    case THING_PWM_DIMMER: {
      if (!state.containsKey("intensity"))
        return;
      int intensity = state.get<int>("intensity");
      analogWrite(thing->pin, (int)((float)intensity * 2.55));
      break;
    }
  }
}











