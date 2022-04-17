#include "client.h"


PostClient::PostClient() {}


PostClient::PostClient(String _host, String _path, String _collection, int _port) {
    host = _host;
    path = _path;
    port = _port;
    collection = _collection;
}


void PostClient::post(String name, int value) {
    HTTPClient http;
    String url = "http://" + host + ":" + String(port) + "/" + path;
    Serial.println(url);
    http.begin(url);
    String value_payload = "{\"" + name + "\": " + String(value) + "}";
    String payload = "{\"collection\": " + collection + ", \"data\": " + value_payload + "}";
    http.POST(payload);

}

