#include "log.h"

TeeLog Log;

size_t TeeLog::write(uint8_t c)
{
  return write(&c, 1);
}

size_t TeeLog::write(const uint8_t *buffer, size_t size)
{
  Serial.write(buffer, size);

  portENTER_CRITICAL(&_mux);
  for (size_t i = 0; i < size; i++)
  {
    if (buffer[i] == '\r') continue;

    _ring[_ringHead++] = (char)buffer[i];
    if (_ringHead >= RING_SIZE)
    {
      _ringHead = 0;
      _ringWrapped = true;
    }
  }
  portEXIT_CRITICAL(&_mux);

  return size;
}

String TeeLog::snapshot()
{
  // Copy out under the lock (fast memcpy), build the String outside it
  char  *copy = (char *)malloc(RING_SIZE);
  if (!copy) return String("(out of memory)\n");

  size_t len;
  bool   wrapped;

  portENTER_CRITICAL(&_mux);
  wrapped = _ringWrapped;
  if (wrapped)
  {
    size_t tail = RING_SIZE - _ringHead;
    memcpy(copy, _ring + _ringHead, tail);
    memcpy(copy + tail, _ring, _ringHead);
    len = RING_SIZE;
  }
  else
  {
    memcpy(copy, _ring, _ringHead);
    len = _ringHead;
  }
  portEXIT_CRITICAL(&_mux);

  // If wrapped, the oldest line is probably partial - start after first '\n'
  size_t start = 0;
  if (wrapped)
  {
    while (start < len && copy[start] != '\n') start++;
    if (start < len) start++;
  }

  String out;
  out.reserve(len - start + 1);
  out.concat(copy + start, len - start);
  free(copy);

  return out;
}
