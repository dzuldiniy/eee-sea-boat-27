#define USE_WIFI_NINA false
#define USE_WIFI101 true
#include <WiFiWebServer.h>
#include <Servo.h>

unsigned long stop_period = 3000000L;

Servo bugservo_l;
Servo bugservo_r;

const char ssid[] = "EEERover";
const char pass[] = "exhibition";
const int groupNumber = 27;  // Set your group number to make the IP address constant - only do this on the EEERover network

const int servopin_dir_r = 8;
const int servopin_en_r = 6;
const int servopin_dir_l = 3;
const int servopin_en_l = 2;
const int radio_pin = A1;
const int ir_pin = 9;
const int magnetic_pin = A2;

//magnetic calibration variables
const double thresholdconst = 0.01;
double thresholdN = 0;
double thresholdS = 0;
const unsigned long calibrationTime = 2000000;

//Webpage to return when root is requested
const char mainwebpage[] = \
"<html><style>\
button{font-size: large; font-family: monospace;}\
.query{font-weight: bold; border: 1px solid black; padding: 15px; font-family: monospace; font-size: large;}\
.query:hover{cursor: pointer}\
.directional{display: grid; justify-content: center;}\
.left,.right,.forward,.reverse{height: 80px; width: 80px;}\
.forward{grid-column-start: 2; grid-row-start: 1;}\
.left{grid-column-start: 1; grid-row-start: 2;}\
.right{grid-column-start: 3; grid-row-start: 2;}\
.reverse{grid-column-start: 2; grid-row-start: 3;}\
</style>\
<body>\
<h1>EEESeaBoat Directional Controller</h1>\
<div class=\"directional\">\
<button class=\"forward\" onmousedown=\"send('up')\" onmouseup=\"send('stop')\">&uarr;</button>\
<button class=\"left\" onmousedown=\"send('left')\" onmouseup=\"send('stop')\">&larr;</button>\
<button class=\"right\" onmousedown=\"send('right')\" onmouseup=\"send('stop')\">&rarr;</button>\
<button class=\"reverse\" onmousedown=\"send('down')\" onmouseup=\"send('stop')\">&darr;</button>\
</div>\
<br><br><button class=\"query\" onmousedown=\"openControls()\">Open Decoders</button>\
</body>\
<script>\
var xhttp = new XMLHttpRequest();\
function send(Parameter) {xhttp.open(\"GET\", \"/\" + Parameter); xhttp.send();}\
function openControls() {window.open(\"/controls\");}\
</script></html>";

const char controlswebpage[] = \
"<html><style>\
body{font-family: monospace; font-size: large;}\
.query{font-weight: bold; border: 1px solid black; padding: 15px; font-family: monospace; font-size: large;}\
.query:hover{cursor: pointer}\
.button{float: left}\
</style><body>\
<button class=\"query\" style=\"font-size: xx-large; padding-inline: 40px;\" onmousedown=\"send('duckidentity')\">Find Duck Identity</button><br><br>\
<button class=\"query\" onmousedown=\"send('name')\">Personalised Name</button><br><br>\
<button class=\"query\" onmousedown=\"send('radio')\">Radio Frequency</button>\
<button class=\"query\" onmousedown=\"send('ir')\">IR Frequency</button><br><br>\
<button class=\"query\" onmousedown=\"send('magnetic')\">Magnetic Direction</button>\
<button class=\"query\" onmousedown=\"send('calibrateMagnetic')\">Calibrate Magnetic Sensor</button>\
<br><br><span id=\"answers\"></span>\
</body><script>\
var xhttp = new XMLHttpRequest();\
let answerdiv = document.getElementById(\"answers\");\
let br = \"<br>\";\
function writeAnswer(Answer) {answerdiv.insertAdjacentHTML(\"afterbegin\", br); answerdiv.insertAdjacentHTML(\"afterbegin\", Answer);}\
xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {writeAnswer(this.responseText);}};\
function send(Parameter) {writeAnswer('Working...'); xhttp.open(\"GET\", \"/\" + Parameter); xhttp.send();}\
</script></html>";


WiFiWebServer server(80);

//Return the web page
void handleRoot() {
  server.send(200, F("text/html"), mainwebpage);
}

void controls() {
  server.send(200, F("text/html"), controlswebpage);
}

void moveup() {
  digitalWrite(servopin_en_r, HIGH);
  digitalWrite(servopin_en_l, HIGH);
  digitalWrite(servopin_dir_l, HIGH);
  digitalWrite(servopin_dir_r, HIGH);
  server.send(200, F("text/plain"), F("forward"));
}

void movedown() {
  digitalWrite(servopin_en_r, HIGH);
  digitalWrite(servopin_en_l, HIGH);
  digitalWrite(servopin_dir_l, LOW);
  digitalWrite(servopin_dir_r, LOW);
  server.send(200, F("text/plain"), F("reverse"));
}

void moveright() {
  digitalWrite(servopin_en_r, HIGH);
  digitalWrite(servopin_dir_r, LOW);
  digitalWrite(servopin_en_l, HIGH);
  digitalWrite(servopin_dir_l, HIGH);
  server.send(200, F("text/plain"), F("right"));
}

void moveleft() {
  digitalWrite(servopin_en_l, HIGH);
  digitalWrite(servopin_en_r, HIGH);
  digitalWrite(servopin_dir_l, LOW);
  digitalWrite(servopin_dir_r, HIGH);
  server.send(200, F("text/plain"), F("left"));
}

void stationary() {
  digitalWrite(servopin_en_r, LOW);
  digitalWrite(servopin_en_l, LOW);
  server.send(200, F("text/plain"), F("stop"));
}

String findName(bool& found) {
  char receivedChar;
  String out = "";
  if (Serial1.available() > 0) {
    if (Serial1.read() == '#') {
      out = "#";
      found = true;
      while(out.length() < 4) {
        receivedChar = Serial1.read();
        if(!isAlpha(receivedChar)){
          found = false;
          return out;
        }
        out += receivedChar;
      }
    } else {
      out = "No name detected";
    }
  } else {
    out = "No signal detected";
  }

  return out;
}

void name() {

  String personalised_name;
  unsigned long start_time = micros();
  bool foundName = false;

  while(micros() - start_time < stop_period && !foundName){
    personalised_name = findName(foundName);
  }

  server.send(200, F("text/plain"), personalised_name);
}

float findFrequency(int pulsepin, bool& found) {
  unsigned long ontime, offtime, duration;
  float out;

  ontime = pulseIn(pulsepin, HIGH);
  delay(100);
  offtime = pulseIn(pulsepin, LOW);
  duration = ontime + offtime;
  out = 1000000/duration;

  if (out > 100 && out < 1100) {
    found = true;
  }

  return out;
}

void frequency(int pulsepin) {
  unsigned long start_time = micros();
  float freq;
  String message;
  bool foundFreq = false;
  
  while(micros() - start_time < stop_period && !foundFreq){
    freq = findFrequency(pulsepin, foundFreq);
  }

  message = freq;
  message += " Hz";

  server.send(200, F("text/plain"), message);
}

void radio() {
  frequency(radio_pin);
}

void ir() {
  frequency(ir_pin);
}

String findMagneticDir(bool& found) {
  String out = "No polarity";
  if(thresholdS != 0) {
    int sensorValue = analogRead(magnetic_pin);

    if(sensorValue > thresholdS) {
      out = "South/Up";
      found = true;
    } else if(sensorValue < thresholdN) {
      out = "North/Down";
      found = true;
    }
  } else {
    out = "Sensor has not been calibrated";
    found = true;
  }

  return out;
}

void magnetic() {

  unsigned long start_time = micros();
  String polarity;
  bool foundMagneticDir = false;

  while (micros() - start_time < stop_period && !foundMagneticDir) {
    polarity = findMagneticDir(foundMagneticDir);
  }

  server.send(200, F("text/plain"), polarity);
}

void calibrateMagnetic() {
  unsigned long start_time = micros();
  int sensorValue, total = 0;
  int count = 0;
  double baseValue;
  String message = "Base value: ";

  while (micros() - start_time < calibrationTime) {
    sensorValue = analogRead(magnetic_pin);
    total += sensorValue;
    count++;
    delay(100);
  }

  baseValue = total / count;
  thresholdS = baseValue + thresholdconst * baseValue;
  thresholdN = baseValue - thresholdconst * baseValue;

  message += baseValue;

  server.send(200, F("text/plain"), message);
}

void findDuckIdentity() {
  bool foundName, foundRadioFreq, foundIRFreq, foundMagneticDir;
  foundName = foundRadioFreq = foundIRFreq = foundMagneticDir = false;
  unsigned long start_time = micros();
  String personalised_name, magneticDir, message;
  float radioFreq, IRFreq;

  while(micros() - start_time < stop_period && !(foundName && foundRadioFreq && foundIRFreq && foundMagneticDir)) {
    if(!foundName){
      personalised_name = findName(foundName);
    }
    if(!foundRadioFreq){
      radioFreq = findFrequency(radio_pin, foundRadioFreq);
    }
    if(!foundIRFreq){
      IRFreq = findFrequency(ir_pin, foundIRFreq);
    }
    if(!foundMagneticDir){
      magneticDir = findMagneticDir(foundMagneticDir);
    }
  }

  message = "Personalised Name: " + personalised_name + "<br>Radio Frequency: " + radioFreq + " Hz<br>IR Frequency: " + IRFreq + " Hz<br>Magnetic Direction: " + magneticDir;

  server.send(200, F("text/plain"), message);

}


//Generate a 404 response with details of the failed request
void handleNotFound() {
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? F("GET") : F("POST");
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, F("text/plain"), message);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(servopin_dir_r, OUTPUT);
  pinMode(servopin_dir_l, OUTPUT);
  pinMode(servopin_en_r, OUTPUT);
  pinMode(servopin_en_l, OUTPUT);
  digitalWrite(servopin_en_l, LOW);
  digitalWrite(servopin_en_r, LOW);
  pinMode(radio_pin, INPUT);
  pinMode(ir_pin, INPUT);
  pinMode(magnetic_pin, INPUT);

  Serial.begin(9600);
  Serial1.begin(600);

  //Wait 10s for the serial connection before proceeding
  //This ensures you can see messages from startup() on the monitor
  //Remove this for faster startup when the USB host isn't attached
  while (!Serial && millis() < 10000)
    ;

  Serial.println(F("\nStarting Web Server"));

  //Check WiFi shield is present
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("WiFi shield not present"));
    while (true)
      ;
  }

  //Configure the static IP address if group number is set
  if (groupNumber)
    WiFi.config(IPAddress(192, 168, 0, groupNumber + 1));

  // attempt to connect to WiFi network
  Serial.print(F("Connecting to WPA SSID: "));
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }

  //Register the callbacks to respond to HTTP requests
  server.on(F("/"), handleRoot);
  server.on(F("/up"), moveup);
  server.on(F("/down"), movedown);
  server.on(F("/right"), moveright);
  server.on(F("/left"), moveleft);
  server.on(F("/stop"), stationary);
  server.on(F("/name"), name);
  server.on(F("/radio"), radio);
  server.on(F("/ir"), ir);
  server.on(F("/magnetic"), magnetic);
  server.on(F("/calibrateMagnetic"), calibrateMagnetic);
  server.on(F("/controls"), controls);
  server.on(F("/duckidentity"), findDuckIdentity);


  server.onNotFound(handleNotFound);

  server.begin();

  Serial.print(F("HTTP server started @ "));
  Serial.println(static_cast<IPAddress>(WiFi.localIP()));
  digitalWrite(LED_BUILTIN, HIGH);
}

//Call the server polling function in the main loop
void loop() {
  server.handleClient();
}
