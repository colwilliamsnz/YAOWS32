#ifndef LOG_H
#define LOG_H

#include <Arduino.h>

/**
 * @brief "Tee" logger used everywhere in place of Serial.
 *
 * Every byte written goes to:
 *   1. the USB serial port (exactly as Serial did before)
 *   2. a 16KB RAM ring buffer (survives light sleep) which the maintenance
 *      window serves as the web log page (http://<hostname>.local/log)
 *
 * Write from the main (Arduino) task. snapshot() is safe to call from the
 * async web server task.
 */
class TeeLog : public Print
{
public:
  size_t write(uint8_t c) override;
  size_t write(const uint8_t *buffer, size_t size) override;
  void   flush() override { Serial.flush(); }

  using Print::write;

  // Buffered history, oldest to newest (partial first line dropped if the
  // buffer has wrapped)
  String snapshot();

private:
  static constexpr size_t RING_SIZE = 16384;

  char   _ring[RING_SIZE];
  size_t _ringHead    = 0;     // next write position
  bool   _ringWrapped = false;

  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
};

extern TeeLog Log;

#endif
