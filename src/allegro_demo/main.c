#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#include "logger/logger.h"

#define SCREEN_W 640
#define SCREEN_H 480
#define TARGET_FPS 60.0

int main(void)
{
    log_set_level(LOG_INFO);
    log_info("CarlsGarage Allegro demo starting");

    if (!al_init()) {
        log_fatal("al_init() failed — cannot initialise Allegro");
        return 1;
    }
    log_info("Allegro %s initialised",
             al_id_to_string(al_get_allegro_version()));

    if (!al_install_keyboard()) {
        log_error("al_install_keyboard() failed");
        return 1;
    }

    if (!al_init_primitives_addon()) {
        log_warn("Primitives addon unavailable — visuals will be minimal");
    }

    al_init_font_addon();

    ALLEGRO_DISPLAY *display = al_create_display(SCREEN_W, SCREEN_H);
    if (!display) {
        log_fatal("Failed to create %dx%d display", SCREEN_W, SCREEN_H);
        return 1;
    }
    al_set_window_title(display, "CarlsGarage — Allegro Demo");
    log_info("Display created (%dx%d)", SCREEN_W, SCREEN_H);

    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
    if (!queue) {
        log_fatal("Failed to create event queue");
        al_destroy_display(display);
        return 1;
    }

    ALLEGRO_TIMER *timer = al_create_timer(1.0 / TARGET_FPS);
    if (!timer) {
        log_fatal("Failed to create timer");
        al_destroy_event_queue(queue);
        al_destroy_display(display);
        return 1;
    }

    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));

    al_start_timer(timer);
    log_info("Event loop running — press Escape or close the window to exit");

    int running = 1;
    int redraw  = 1;
    float hue   = 0.0f;

    while (running) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        switch (ev.type) {
            case ALLEGRO_EVENT_TIMER:
                hue = hue + 0.5f;
                if (hue >= 360.0f) hue -= 360.0f;
                redraw = 1;
                break;

            case ALLEGRO_EVENT_KEY_DOWN:
                if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    log_info("Escape pressed — shutting down");
                    running = 0;
                }
                break;

            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                log_info("Window closed — shutting down");
                running = 0;
                break;

            default:
                break;
        }

        if (redraw && al_is_event_queue_empty(queue)) {
            redraw = 0;
            /* Simple animated background: cycle through hues */
            float r, g, b;
            /* Convert HSV (hue, 0.6, 0.9) → RGB the quick way */
            float h6 = hue / 60.0f;
            int   i  = (int)h6 % 6;
            float f  = h6 - (int)h6;
            float p  = 0.9f * (1.0f - 0.6f);
            float q  = 0.9f * (1.0f - 0.6f * f);
            float t2 = 0.9f * (1.0f - 0.6f * (1.0f - f));
            switch (i) {
                case 0: r=0.9f; g=t2;   b=p;    break;
                case 1: r=q;    g=0.9f; b=p;    break;
                case 2: r=p;    g=0.9f; b=t2;   break;
                case 3: r=p;    g=q;    b=0.9f; break;
                case 4: r=t2;   g=p;    b=0.9f; break;
                default:r=0.9f; g=p;    b=q;    break;
            }
            al_clear_to_color(al_map_rgb_f(r, g, b));
            al_flip_display();
        }
    }

    log_info("Cleaning up");
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(display);
    log_info("Goodbye from CarlsGarage!");
    return 0;
}
