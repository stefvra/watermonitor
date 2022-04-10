#pragma once
#include <Arduino.h>
#include <HTTPClient.h>


class PostClient {
    private:
    String host = "localhost";
    int port = 80;
    String path = "/";
    public:
    PostClient();
    PostClient(String _host, String _path, int _port);
    void post(String name, int value);
};