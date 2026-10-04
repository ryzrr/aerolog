#pragma once

void netBegin();                                        // blocks until WiFi is up (boot only)
void netLoop();                                         // FR3: reconnect + mqtt.loop(), non-blocking
bool netPublish(const char* topic, const char* msg);    // false if not connected
