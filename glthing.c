#include <epoxy/gl.h>
#include <SDL.h>
#include <stdlib.h>
#include <stdbool.h>

struct glthing {
        SDL_Window *window;
        SDL_GLContext gl_context;

        bool quit;
};

static void
redraw(struct glthing *glthing)
{
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT);

        SDL_GL_SwapWindow(glthing->window);
}

static void
process_window_event(struct glthing *glthing,
                     const SDL_WindowEvent *event)
{
        switch (event->event) {
        case SDL_WINDOWEVENT_EXPOSED:
                redraw(glthing);
                break;

        case SDL_WINDOWEVENT_CLOSE:
                glthing->quit = true;
                break;
        }
}

static void
process_event(struct glthing *glthing,
              const SDL_Event *event)
{
        switch (event->type) {
        case SDL_WINDOWEVENT:
                process_window_event(glthing, &event->window);
                break;

        case SDL_QUIT:
                glthing->quit = true;
                break;
        }
}

static bool
main_loop(struct glthing *glthing)
{
        SDL_Event event;
        int res;

        while (!glthing->quit) {
                res = SDL_WaitEvent(&event);
                if (res < 0) {
                        fprintf(stderr, "SDL_WaitEvent: %s\n", SDL_GetError());
                        return false;
                }

                process_event(glthing, &event);
        }

        return true;
}

int
main(int argc, char **argv)
{
        int ret = EXIT_SUCCESS;
        struct glthing glthing;
        int res;

        memset(&glthing, 0, sizeof glthing);

        res = SDL_Init(SDL_INIT_VIDEO);
        if (res < 0) {
                fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
                ret = EXIT_FAILURE;
                goto out;
        }

        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_CORE);

        glthing.window = SDL_CreateWindow("glthing",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          640, 480,
                                          SDL_WINDOW_OPENGL);

        if (glthing.window == NULL) {
                fprintf(stderr,
                        "Failed to create SDL window: %s\n",
                        SDL_GetError());
                ret = EXIT_FAILURE;
                goto out_sdl;
        }

        glthing.gl_context = SDL_GL_CreateContext(glthing.window);
        if (glthing.gl_context == NULL) {
                fprintf(stderr,
                        "Failed to create GL context window: %s\n",
                        SDL_GetError());
                ret = EXIT_FAILURE;
                goto out_window;
        }

        SDL_GL_MakeCurrent(glthing.window, glthing.gl_context);

        if (!main_loop(&glthing))
                ret = EXIT_FAILURE;

        SDL_GL_MakeCurrent(NULL, NULL);
        SDL_GL_DeleteContext(glthing.gl_context);
out_window:
        SDL_DestroyWindow(glthing.window);
out_sdl:
        SDL_Quit();
out:
        return ret;
}
