#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "engine.h"

#define WIDTH 600
#define HEIGHT 600

#define FPS_MEASURE_LEN 120

typedef struct FPSCounter {
    float data[FPS_MEASURE_LEN];
    float prev;
    int i;
} FPSCounter;

float fps_append_and_measure(FPSCounter *f, float m) {
    float sum_prev = f->prev - f->data[f->i];
    f->data[f->i] = m;
    f->i++;
    if (f->i == FPS_MEASURE_LEN) {
        f->i = 0;
    }
    float new_sum = sum_prev + m;
    f->prev = new_sum;
    return new_sum / FPS_MEASURE_LEN;
}

float diff_time_ms(struct timespec *t1) {
    struct timespec t2;
    clock_gettime(CLOCK_MONOTONIC, &t2);

    float time_spent = (t2.tv_sec - t1->tv_sec) * 1000.0 + 
                       (t2.tv_nsec - t1->tv_nsec) / 1000000.0;
    
    *t1 = t2;

    return time_spent;
}

int main() {
    // I want to see output before segmentation fault
    setbuf(stdout, NULL);

    // Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        // Handle initialization failure
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("SDL Event Handling",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          WIDTH,
                                          HEIGHT,
                                          SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    if (!window) {
        // Handle window creation failure
        SDL_Quit();
        return -1;
    }

    Engine engine;
    engine_init_sdl(&engine, WIDTH, HEIGHT, window);

    struct timespec delta_timer, debug_timer;
    clock_gettime(CLOCK_MONOTONIC, &delta_timer);
    clock_gettime(CLOCK_MONOTONIC, &debug_timer); 

    // struct timespec sleep_t = {
    //     .tv_nsec = 1 * 1000000,
    // };

    FPSCounter counter = {0};

    int running = 1;

    int width = WIDTH;
    int height = HEIGHT;

    float min_cycle_ms = 500;
    float max_cycle_ms = 5000;

    float accum_cycle = 0;
    float cur_cycle = max_cycle_ms;

    int mouse_inside = 0;
    int mouse_x = 0;
    int mouse_y = 0;

    char window_title[64];

    SDL_Event event;

    while (running) {
        float delta_ms = diff_time_ms(&delta_timer);
        // float render_ms = diff_time_ms(&debug_timer);
        // printf("render_ms %.2f ms\n", render_ms);
        // printf("Time elapsed: %.2f ms\n", delta_ms);

        {
            float val = fps_append_and_measure(&counter, 1000.0f / delta_ms);
            snprintf(window_title, sizeof(window_title), "FPS: %.2f", (double) val);
            SDL_SetWindowTitle(window, window_title);
        }

        // oval distance
        {
            float alpha;

            if (mouse_inside) {
                float dx = width / 2.0f - mouse_x;
                float dy = height / 2.0f - mouse_y;
                float maxdx = (width / 2.0f) * (width / 2.0f);
                float maxdy = (height / 2.0f) * (width / 2.0f);
                float diag = dx * dx / maxdx + dy * dy / maxdy;

                alpha = fmin(1.0f, diag);
            } else {
                alpha = 1.0f;
            }

            cur_cycle = min_cycle_ms + (max_cycle_ms - min_cycle_ms) * alpha;
        }

        accum_cycle += delta_ms / cur_cycle;
        if (accum_cycle > 1.0f) {
            accum_cycle -= 1.0f;
        }

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = 0;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = 0;
                }
                break;
            case SDL_MOUSEMOTION:
                // Handle mouse movement here
                mouse_x = event.motion.x;
                mouse_y = event.motion.y;
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                    width = event.window.data1;
                    height = event.window.data2;
                    engine_signal_resize(&engine, width, height);
                    break;
                case SDL_WINDOWEVENT_ENTER:
                    mouse_inside = 1;
                    break;
                case SDL_WINDOWEVENT_LEAVE:
                    mouse_inside = 0;
                    break;
                }
                break;
            }
        }

        if (!running) {
            break;
        }
        
        // float x_ms = diff_time_ms(&debug_timer);
        // printf("x_ms %.2f ms\n", x_ms);

        engine_draw(&engine, accum_cycle);

        // nanosleep(&sleep_t, NULL);
    }

    engine_deinit(&engine);

    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();

    return 0;
}