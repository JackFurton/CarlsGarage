#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#include "logger/logger.h"
#include "logger/throttled.h"

#include <math.h>
#include <stdio.h>

/* ---------------------------------------------------------------------------
 * Display / timing constants
 * -------------------------------------------------------------------------*/
#define SCREEN_W     640
#define SCREEN_H     480
#define TARGET_FPS   60.0

/* ---------------------------------------------------------------------------
 * Ball entity
 * -------------------------------------------------------------------------*/
typedef struct {
    float x, y;    /* centre position */
    float vx, vy;  /* velocity in pixels per second */
    float radius;
} Ball;

static void ball_init(Ball *b)
{
    b->x      = SCREEN_W / 2.0f;
    b->y      = SCREEN_H / 2.0f;
    b->vx     = 180.0f;
    b->vy     = 120.0f;
    b->radius = 20.0f;
}

/* Fixed-timestep update: move ball and bounce off walls */
static void ball_update(Ball *b, float dt)
{
    b->x += b->vx * dt;
    b->y += b->vy * dt;

    if (b->x - b->radius < 0.0f) {
        b->x  = b->radius;
        b->vx = fabsf(b->vx);
    } else if (b->x + b->radius > SCREEN_W) {
        b->x  = SCREEN_W - b->radius;
        b->vx = -fabsf(b->vx);
    }

    if (b->y - b->radius < 0.0f) {
        b->y  = b->radius;
        b->vy = fabsf(b->vy);
    } else if (b->y + b->radius > SCREEN_H) {
        b->y  = SCREEN_H - b->radius;
        b->vy = -fabsf(b->vy);
    }
}

static void ball_draw(const Ball *b)
{
    al_draw_filled_circle(b->x, b->y, b->radius,
                          al_map_rgb(255, 220, 50));
    al_draw_circle(b->x, b->y, b->radius,
                   al_map_rgb(200, 160, 20), 2.0f);
}

/* ---------------------------------------------------------------------------
 * FPS counter
 * -------------------------------------------------------------------------*/
typedef struct {
    unsigned long frame_count;   /* total frames rendered */
    double        accum;         /* seconds since last fps sample */
    double        last_fps;      /* most recently computed fps */
} FpsCounter;

static void fps_tick(FpsCounter *fc, double elapsed)
{
    fc->frame_count++;
    fc->accum += elapsed;
    if (fc->accum >= 1.0) {
        fc->last_fps  = fc->frame_count / fc->accum;
        fc->frame_count = 0;
        fc->accum     = 0.0;
    }
}

/* ---------------------------------------------------------------------------
 * Background hue animation (carried over from original demo)
 * -------------------------------------------------------------------------*/
static void hue_to_rgb(float hue, float *r, float *g, float *b)
{
    float h6 = hue / 60.0f;
    int   i  = (int)h6 % 6;
    float f  = h6 - (int)h6;
    float p  = 0.9f * (1.0f - 0.6f);
    float q  = 0.9f * (1.0f - 0.6f * f);
    float t2 = 0.9f * (1.0f - 0.6f * (1.0f - f));
    switch (i) {
        case 0: *r=0.9f; *g=t2;   *b=p;    break;
        case 1: *r=q;    *g=0.9f; *b=p;    break;
        case 2: *r=p;    *g=0.9f; *b=t2;   break;
        case 3: *r=p;    *g=q;    *b=0.9f; break;
        case 4: *r=t2;   *g=p;    *b=0.9f; break;
        default:*r=0.9f; *g=p;    *b=q;    break;
    }
}

/* ---------------------------------------------------------------------------
 * Main
 * -------------------------------------------------------------------------*/
int main(void)
{
    log_set_level(LOG_INFO);
    log_info("CarlsGarage Allegro demo starting");

    /* --- Allegro core init --- */
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
        log_warn("Primitives addon unavailable — ball will not be drawn");
    }

    al_init_font_addon();
    ALLEGRO_FONT *font = al_create_builtin_font();
    if (!font) {
        log_warn("Built-in font unavailable — HUD text will be skipped");
    }

    /* --- Display --- */
    ALLEGRO_DISPLAY *display = al_create_display(SCREEN_W, SCREEN_H);
    if (!display) {
        log_fatal("Failed to create %dx%d display", SCREEN_W, SCREEN_H);
        return 1;
    }
    al_set_window_title(display, "CarlsGarage — Game Loop Demo");
    log_info("Display created (%dx%d)", SCREEN_W, SCREEN_H);

    /* --- Event queue & timer --- */
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

    /* --- Game state --- */
    Ball       ball;
    FpsCounter fps = {0};
    float      hue = 0.0f;
    int        running = 1;
    int        redraw  = 0;
    int        paused  = 0;

    ball_init(&ball);

    const double dt = 1.0 / TARGET_FPS;  /* fixed timestep matches timer tick */

    al_start_timer(timer);
    log_info("Game loop running — Escape/close to quit, P to pause");

    /* -----------------------------------------------------------------------
     * Game loop
     *
     * Pattern: fixed-timestep update driven by a timer event, render only
     * when the queue is drained so we never queue up redundant frames.
     * This is the classic "semi-fixed timestep" used in many small games:
     * one timer tick  →  one physics update  →  one render pass.
     * ---------------------------------------------------------------------*/
    while (running) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        switch (ev.type) {

            case ALLEGRO_EVENT_TIMER:
                if (!paused) {
                    ball_update(&ball, (float)dt);
                    hue += 0.3f;
                    if (hue >= 360.0f) hue -= 360.0f;
                }
                redraw = 1;
                break;

            case ALLEGRO_EVENT_KEY_DOWN:
                switch (ev.keyboard.keycode) {
                    case ALLEGRO_KEY_ESCAPE:
                        log_info("Escape pressed — shutting down");
                        running = 0;
                        break;
                    case ALLEGRO_KEY_P:
                        paused = !paused;
                        log_info("Game %s", paused ? "paused" : "resumed");
                        break;
                    case ALLEGRO_KEY_R:
                        ball_init(&ball);
                        log_info("Ball reset");
                        break;
                    default:
                        break;
                }
                break;

            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                log_info("Window closed — shutting down");
                running = 0;
                break;

            default:
                break;
        }

        /* ---------------------------------------------------------------
         * Render — only when we have a pending redraw AND the queue is
         * drained, so we never draw a stale frame while events pile up.
         * -------------------------------------------------------------*/
        if (redraw && al_is_event_queue_empty(queue)) {
            redraw = 0;

            fps_tick(&fps, dt);

            /* Throttled log: emit FPS to the logger at most once per second */
            throttled(1, log_debug("FPS: %.1f  ball=(%.0f,%.0f)",
                                   fps.last_fps, ball.x, ball.y));

            /* Background */
            float r, g, b;
            hue_to_rgb(hue, &r, &g, &b);
            al_clear_to_color(al_map_rgb_f(r, g, b));

            /* Ball */
            ball_draw(&ball);

            /* HUD overlay */
            if (font) {
                char hud[64];
                snprintf(hud, sizeof(hud), "FPS: %.1f  |  P=pause  R=reset  Esc=quit",
                         fps.last_fps);
                al_draw_text(font, al_map_rgb(255, 255, 255),
                             4, 4, 0, hud);

                if (paused) {
                    al_draw_text(font, al_map_rgb(255, 80, 80),
                                 SCREEN_W / 2 - 20, SCREEN_H / 2, 0,
                                 "PAUSED");
                }
            }

            al_flip_display();
        }
    }

    /* --- Teardown --- */
    log_info("Cleaning up");
    if (font)    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    al_destroy_display(display);
    log_info("Goodbye from CarlsGarage!");
    return 0;
}
