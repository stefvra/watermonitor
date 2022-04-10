#include "client.h"


PostClient::PostClient() {}


PostClient::PostClient(String _host, String _path, int _port) {
    host = _host;
    path = _path;
    port = _port;
}


void PostClient::post(String name, int value) {
    HTTPClient http;
    String url = "http://" + host + ":" + String(port) + "/" + path;
    Serial.println(url);
    http.begin(url);
    String payload = "{\"" + name + "\": " + String(value) + "}";
    http.POST(payload);

}

