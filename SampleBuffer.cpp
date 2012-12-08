#include "ch.h"
#include "SampleBuffer.h"

SampleBuffer::SampleBuffer()
  : tp_(0), i_(0)
{
  tp_ = chThdCreateStatic(waWriteThread,
                          sizeof(waWriteThread),
                          NORMALPRIO, WriteThread_, NULL);
}

msg_t SampleBuffer::WriteThread()
{
  chRegSetThreadName("WriteThread");

  while (1) {
    Thread * messaging_tp = chMsgWait();
    msg_t message = chMsgGet(messaging_tp);
    FRESULT res = FR_OK;
    if (message == 0) { // increment to next Sample in buffer
      ++i_;

      if ((i_ == NUMBER_OF_SAMPLES) || (i_ == NUMBER_OF_SAMPLES / 2)) {
        if (i_ == NUMBER_OF_SAMPLES)
          i_ = 0;

        UINT bytes;
        f_write(&f_, reinterpret_cast<uint8_t *>(BackBuffer()),
                sizeof(Sample)*(NUMBER_OF_SAMPLES/2), &bytes);
        f_sync(&f_);
      }
    } else if (message == -1) { // reset the buffer, close file, toss whatever is in the front buffer (implies we lose at most NUMBER_OF_SAMPLES/2)
      i_ = 0;
      res = f_close(&f_);
    } else {  // open a new file; ASSUMPTION: if the message isn't 0 or -1, it is a pointer to char with the filename.  This should always work as long as you couldn't have a char array located at address 0xFFFFFFFF which is equivalent to -1
      res = f_open(&f_, reinterpret_cast<const char *>(message), FA_CREATE_ALWAYS | FA_WRITE);
    }
    chMsgRelease(messaging_tp, res);  // return the result of file operation, or zero for increment
  } // while

  return 0;
}
