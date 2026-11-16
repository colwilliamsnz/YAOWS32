#include "log.h"

// *** Global Log object (see log.h) *******************************************
TeeLog Log;


/**
 * @brief Print one byte (Print class requirement) - see write(buffer, size)
 */
size_t TeeLog::write(uint8_t c)
{
  return write(&c, 1);
} // end function TeeLog::write (byte) *****************************************


/**
 * @brief Print a block of bytes: send to the USB serial port, then copy into
 * the history ring buffer for the /log page. Carriage returns are dropped from
 * the history (browsers only need \n).
 */
size_t TeeLog::write(const uint8_t *buffer, size_t size)
{
  Serial.write(buffer, size);

  // ring buffer is shared with the web server task so lock it while writing
  portENTER_CRITICAL(&_mux);

  for (size_t i = 0; i < size; i++)
  {
    if (buffer[i] == '\r') continue;

    _ring[_ringHead++] = (char)buffer[i];

    // reached the end - wrap round and overwrite the oldest history
    if (_ringHead >= RING_SIZE)
    {
      _ringHead = 0;
      _ringWrapped = true;
    }
  }

  portEXIT_CRITICAL(&_mux);

  return size;
} // end function TeeLog::write (buffer) ***************************************


/**
 * @brief Return the history as one String, oldest to newest, for the /log
 * page. If the buffer has wrapped, the oldest line is probably cut off so it
 * is dropped.
 */
String TeeLog::snapshot()
{
  // *** Copy the ring out while locked (fast), build the String after *********
  char *ringCopy = (char *)malloc(RING_SIZE);
  if (!ringCopy) return String("(out of memory)\n");

  size_t copyLen;
  bool   wrapped;

  portENTER_CRITICAL(&_mux);

  wrapped = _ringWrapped;

  if (wrapped)
  {
    // oldest data starts at _ringHead - copy from there to the end, then the
    // start of the buffer up to _ringHead
    size_t tailLen = RING_SIZE - _ringHead;
    memcpy(ringCopy, _ring + _ringHead, tailLen);
    memcpy(ringCopy + tailLen, _ring, _ringHead);
    copyLen = RING_SIZE;
  }
  else
  {
    memcpy(ringCopy, _ring, _ringHead);
    copyLen = _ringHead;
  }

  portEXIT_CRITICAL(&_mux);

  // *** If wrapped, skip the (probably partial) oldest line *******************
  size_t start = 0;

  if (wrapped)
  {
    while (start < copyLen && ringCopy[start] != '\n') start++;
    if (start < copyLen) start++;
  }

  String history;
  history.reserve(copyLen - start + 1);
  history.concat(ringCopy + start, copyLen - start);

  free(ringCopy);

  return history;
} // end function TeeLog::snapshot *********************************************
