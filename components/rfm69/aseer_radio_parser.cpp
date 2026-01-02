#include "aseer_radio_parser.h"

#include <Arduino.h>
#include "rfm69.h"

using esphome::rfm69::QueuedPacket;

static int __last_state = 0;

static long __cur_pulse_start = 0;
static long __cur_pulse_width = 0;
static long __cur_pulse_cycle = 0;

static long __bk_pulse_start = 0;

static byte __cur_mask;
static byte *__cur_out;
static byte *__cur_end;

static QueuedPacket _buffer1;
static QueuedPacket _buffer2;
static QueueHandle_t _aseerPacketOutputQueue = nullptr;

static bool _buffTog;

static void aseer_flip_page();

void aseer_set_output_queue(QueueHandle_t queue)
{
  _aseerPacketOutputQueue = queue;
  _buffer1.len = 0;
  _buffer2.len = 0;
  aseer_flip_page();
}

static void aseer_flip_page()
{
  byte *pending_out = __cur_out;
  __cur_out = _buffTog ? _buffer2.data : _buffer1.data;

  __cur_end = __cur_out + RFM_SENSOR_BUFFER_SIZE;
  __cur_mask = 128;
  *__cur_out = 0;

  _buffTog = !_buffTog;

  auto new_pending_buffer = _buffTog ? &_buffer2 : &_buffer1;
  new_pending_buffer->len = pending_out - new_pending_buffer->data;
  if (new_pending_buffer->len > 2)
  {
    xQueueSendToFrontFromISR(_aseerPacketOutputQueue, new_pending_buffer, 0);
  }
}

void aseer_handle_state_interrupt(int state)
{
  long tm = micros();

  // Rising Edge
  if (__last_state == 0 && state == 1)
  {
    // We have a previous rising edge
    if (__cur_pulse_start)
    {

      // Current pulse cycle width
      __cur_pulse_cycle = tm - __cur_pulse_start;
    }

    // Backup the current pulse start time for glitch filtering
    __bk_pulse_start = __cur_pulse_start;

    __cur_pulse_start = tm;
  }

  // Falling edge
  else if (__last_state == 1 && state == 0)
  {
    // Record current pulse width
    long new_pulse_width = tm - __cur_pulse_start;
    if (new_pulse_width >= 200)
    {
      if (__cur_pulse_cycle)
      {
        // Process previous pulse

        // Is this a 1 or a 0?
        if (__cur_pulse_width > __cur_pulse_cycle / 3)
        {
          *__cur_out |= __cur_mask;
        }

        // If the pulse was big enough -- This is a new packet
        bool end_packet = __cur_pulse_cycle > 5000;

        if (!end_packet)
        {
          // Move to next bit
          __cur_mask = __cur_mask >> 1;

          // .. Or byte..
          if (!__cur_mask)
          {
            __cur_mask = 128;
            __cur_out++;

            if (__cur_out < __cur_end)
            {
              *__cur_out = 0;
            }
            else
            {
              // .. Or packet
              end_packet = true;
            }
          }
        }

        if (end_packet)
        {
          aseer_flip_page();
        }
      }

      // Update new pulse width
      __cur_pulse_width = new_pulse_width;
    }
    else
    {
      __cur_pulse_start = __bk_pulse_start;
    }
  }

  __last_state = state;
}