#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Arduino.h>

//network constants
const char *ssid = ".....";
const char *password = ".....";

// thermistor constants
const double VCC = 3.3;             // NodeMCU on board 3.3v vcc
const double R2 = 2080;            // 10k ohm series resistor
const double adc_resolution = 1023; // 10-bit adc
const float A = 1.009249522e-03, B = 2.378405444e-04, C = 2.019202697e-07;


ESP8266WebServer server(80);

const int led = 13;

void handleRoot() {
    digitalWrite(led, 1);
    server.send(200, "text/plain", "hello from GarageController nr1!");
    digitalWrite(led, 0);
}

void Heater() {
    if (server.method() != HTTP_PUT) {
        digitalWrite(led, 1);
        server.send(405, "text/plain", "Method Not Allowed");
        digitalWrite(led, 0);
    } else {
        digitalWrite(led, 1);
        server.send(200, "text/plain", "Heater: " + server.arg(0));
        digitalWrite(led, 0);
    }
}

void Temperature() {
    if (server.method() != HTTP_GET) {
        digitalWrite(led, 1);
        server.send(405, "text/plain", "Method Not Allowed");
        digitalWrite(led, 0);
    } else {
        digitalWrite(led, 1);
        double Vout, Rth, temperature, adc_value;
        adc_value = analogRead(A0);
        Vout = (adc_value * VCC) / adc_resolution;
        Rth = (VCC * R2 / Vout) - R2;

        /*  Steinhart-Hart Thermistor Equation:
         *  Temperature in Kelvin = 1 / (A + B[ln(R)] + C[ln(R)]^3)
        *  where A = 0.001129148, B = 0.000234125 and C = 8.76741*10^-8  */
        temperature = (1 / (A + (B * log(Rth)) + (C * pow((log(Rth)), 3))));   // Temperature in kelvin

        temperature = temperature - 273.15;  // Temperature in degree celsius
        temperature = (temperature - 32) * 0.5556;
        server.send(200, "application/json", "Temperature: " + String(temperature));
        digitalWrite(led, 0);
    }
}

void handleNotFound() {
    digitalWrite(led, 1);
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
    digitalWrite(led, 0);
}

void setup(void) {
    pinMode(led, OUTPUT);
    digitalWrite(led, 0);
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp8266")) {
        Serial.println("MDNS responder started");
    }

    server.on("/", handleRoot);

    server.on("/Heater/", Heater);

    server.on("/Temperature/", Temperature);

    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("HTTP server started");
}

void loop(void) {
    server.handleClient();
}