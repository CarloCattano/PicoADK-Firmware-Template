#include <stdio.h>
#include "project_config.h"

// Reverse compatibility with old PicoSDK.
#if __has_include("bsp/board_api.h")
#include "bsp/board_api.h"
#else
#include "bsp/board.h"
#endif

#include "midi_input_usb.h"
#include "audio_subsystem.h"
#include "picoadk_hw.h"


// TODO libpd m_pd.h not found
// #include "z_libpd.h"

#include "FreeRTOS.h"
#include <task.h>
#include <queue.h>

#include "arduino_compat.h"

// Audio Buffer (Size is set in lib/audio/include/audio_subsystem.h)
audio_buffer_pool_t *audio_pool;

MIDIInputUSB usbMIDI;

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * Task to handle USB MIDI input processing.
     *
     * @param pvParameters Unused task parameters
     */
    void usb_midi_task(void *pvParameters)
    {
        // Setup MIDI Callbacks using lambdas
        usbMIDI.setCCCallback([](uint8_t cc, uint8_t value, uint8_t channel) {
            // Handle Control Change (CC) event
            // e.g. Dsp_cc(ctx, cc, value, channel);
        });

        usbMIDI.setNoteOnCallback([](uint8_t note, uint8_t velocity, uint8_t channel) {
            if (velocity > 0)
            {
                // Handle Note On event
                // e.g. Dsp_noteOn(ctx, note, channel);
            }
            else
            {
                // Treat zero velocity as Note Off
                // e.g. Dsp_noteOff(ctx, note, channel);
            }
        });

        usbMIDI.setNoteOffCallback([](uint8_t note, uint8_t velocity, uint8_t channel) {
            // Handle Note Off event
            // e.g. Dsp_noteOff(ctx, note, channel);
        });

        while (1)
        {
            // TinyUSB Device Task
            #if (OPT_MODE_HOST == 1)
            tuh_task();
            #else
            tud_task();
            #endif
            usbMIDI.process();
        }
    }

    /**
     * Task to blink an LED on GPIO pin 2.
     *
     * @param pvParameters Unused task parameters
     */
    void blinker_task(void *pvParameters)
    {
        gpio_init(2);
        gpio_set_dir(2, GPIO_OUT);

        while (1)
        {
            gpio_put(2, 1);
            vTaskDelay(pdMS_TO_TICKS(100)); // Delay for 100ms
            gpio_put(2, 0);
        }
    }

    /**
     * Main entry point.
     */


    int main(void)
    {
        // Initialize hardware
        picoadk_init();
        
        // Initialize libpd
        // libpd_init();
        
        // Initialize DSP engine (if needed)

        // Initialize the audio subsystem
        audio_pool = init_audio();

        // Create FreeRTOS tasks for MIDI handling and LED blinking
        xTaskCreate(usb_midi_task, "USB_MIDI_Task", 4096, NULL, configMAX_PRIORITIES, NULL);
        xTaskCreate(blinker_task, "Blinker_Task", 128, NULL, configMAX_PRIORITIES - 1, NULL);

        // Start the FreeRTOS scheduler
        vTaskStartScheduler();

        // Idle loop (this is fine for Cortex-M33)
        while (1)
        {
            // Could use `taskYIELD()` or similar here if needed
        }
    }

    /**
     * I2S audio callback for filling the audio buffer with samples.
     *
     * This function is called at a fixed rate determined by the audio subsystem
     * and must return within the interval between calls to avoid audio glitches.
     */
    void __not_in_flash_func(i2s_callback_func())
    {
        audio_buffer_t *buffer = take_audio_buffer(audio_pool, false);
        if (buffer == NULL)
        {
            return;
        }

        int32_t *samples = (int32_t *)buffer->buffer->bytes;

        // Fill buffer with 32-bit samples (stereo, 2 channels)
        for (uint i = 0; i < buffer->max_sample_count; i++)
        {
            samples[i * 2 + 0] = 0;   // Left channel sample
            samples[i * 2 + 1] = 0;   // Right channel sample
            // Use your DSP function here for generating the audio samples
        }

        buffer->sample_count = buffer->max_sample_count;
        give_audio_buffer(audio_pool, buffer);
    }



/*
  test libpd by running a patch in a loop
*/
// #include "z_libpd.h"
//
//
// int main(int argc, char **argv) {
//
//   libpd_set_noteonhook(pdnoteon);
//   libpd_init();
//   libpd_init_audio(1, 2, srate);
//   float inbuf[64], outbuf[128];  // one input channel, two output channels
//                                  // block size 64, one tick per buffer
//
//   // compute audio    [; pd dsp 1(
//   libpd_start_message(1); // one entry in list
//   libpd_add_float(1.0f);
//   libpd_finish_message("pd", "dsp");
//
//   // open patch       [; pd open file folder(
  // if (!libpd_openfile("test.pd", NULL))
//     return -1;
//
//   // now run pd for ten seconds (logical time)
//   int i;
//   for (i = 0; i < 10 * srate / 64; i++) {
//     // fill inbuf here
//     libpd_process_float(1, inbuf, outbuf);
//     // use outbuf here
//   }
//   for (i = 0; i < 10; i++)
//     printf("%g\n", outbuf[i]);
//   return 0;
// }

#ifdef __cplusplus
}
#endif
