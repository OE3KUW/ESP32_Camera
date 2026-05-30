/***************************************************************************************
 *
 *                              E S P 3 2   C a m e r a
 *
 *                                                        kuran june 2026
 ***************************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "esp_camera.h"

WebServer server(80);

// AI-Thinker ESP32-CAM Pins
#define PWDN  32
#define RESET -1
#define XCLK   0
#define SIOD  26
#define SIOC  27
#define Y9    35
#define Y8    34
#define Y7    39
#define Y6    36
#define Y5    21
#define Y4    19
#define Y3    18
#define Y2     5
#define VSYNC 25
#define HREF  23
#define PCLK  22

void startCamera()
{
    camera_config_t c = {};

    c.ledc_channel = LEDC_CHANNEL_0;
    c.ledc_timer   = LEDC_TIMER_0;

    c.pin_d0 = Y2;
    c.pin_d1 = Y3;
    c.pin_d2 = Y4;
    c.pin_d3 = Y5;
    c.pin_d4 = Y6;
    c.pin_d5 = Y7;
    c.pin_d6 = Y8;
    c.pin_d7 = Y9;

    c.pin_xclk  = XCLK;
    c.pin_pclk  = PCLK;
    c.pin_vsync = VSYNC;
    c.pin_href  = HREF;

    c.pin_sscb_sda = SIOD;
    c.pin_sscb_scl = SIOC;

    c.pin_pwdn  = PWDN;
    c.pin_reset = RESET;

    c.xclk_freq_hz = 20000000;
    c.pixel_format = PIXFORMAT_JPEG;

    // Für flüssigeres Streaming kleiner wählen
    c.frame_size   = FRAMESIZE_QQVGA;   // 160x120, flüssiger
    // c.frame_size = FRAMESIZE_QVGA;    // 320x240, schöner aber langsamer

    c.jpeg_quality = 14;                // kleiner = bessere Qualität, aber langsamer/größer
    c.fb_count     = 2;                 // 2 Frames für flüssigeren Stream

    esp_err_t err = esp_camera_init(&c);

    if (err != ESP_OK) {
        Serial.printf("Camera init failed: 0x%x\n", err);
        while (true) delay(1000);
    }

    Serial.println("Camera OK");
}

void jpg()
{
    camera_fb_t* fb = esp_camera_fb_get();

    if (!fb) {
        server.send(500, "text/plain", "camera error");
        return;
    }

    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "image/jpeg", (const char*)fb->buf, fb->len);

    esp_camera_fb_return(fb);
}

void stream()
{
    WiFiClient client = server.client();

    client.print(
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        "Cache-Control: no-cache\r\n"
        "Pragma: no-cache\r\n"
        "Connection: close\r\n\r\n"
    );

    while (client.connected()) {
        camera_fb_t* fb = esp_camera_fb_get();

        if (!fb) {
            Serial.println("frame failed");
            break;
        }

        client.print("--frame\r\n");
        client.print("Content-Type: image/jpeg\r\n");
        client.print("Content-Length: ");
        client.print(fb->len);
        client.print("\r\n\r\n");

        client.write(fb->buf, fb->len);
        client.print("\r\n");

        esp_camera_fb_return(fb);

        delay(40);   // ca. 20–25 Bilder/s möglich, je nach Board
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("### PROGRAMM STARTET ###");

    LittleFS.begin();

    startCamera();

    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-CAM", "");

    Serial.print("AP-IP: ");
    Serial.println(WiFi.softAPIP());

    server.serveStatic("/", LittleFS, "/index.html");
    server.serveStatic("/style.css", LittleFS, "/style.css");

    server.on("/jpg", jpg);
    server.on("/stream", stream);

    server.begin();

    Serial.println("Webserver gestartet");
}

void loop()
{
    server.handleClient();
}