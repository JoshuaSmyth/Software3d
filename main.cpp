#include <stdlib.h>
#include <SDL2/SDL.h>
#include <cmath>
#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif
#undef main

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Surface *surface;

#define INTERNAL_WIDTH 320
#define INTERNAL_HEIGHT 200

#define PIXEL_SCALE 3


#define M_PI           3.14159265358979323846 
#define M_PI_2         1.57079632679489661923
#define M_2PI          2*M_PI
#define FPS_INTERVAL 1.0 //seconds.

Uint32 fps_lasttime = SDL_GetTicks();
Uint32 fps_current;
Uint32 fps_frames = 0;

// TODO Combine into camera struct
float cameraRotationRadians = 0;
float cameraZoom = 0;

struct vec2_uint16
{
    Uint16 X;
    Uint16 Y;

public:
    vec2_uint16()
    {
        X = 0;
        Y = 0;
    }

    vec2_uint16(const vec2_uint16& v)
    {
        X = v.X;
        Y = v.Y;
    }

    vec2_uint16(Uint16 x, Uint16 y)
    {
        X = x; Y = y;
    }
};

struct line2_uint16
{
    vec2_uint16 A;
    vec2_uint16 B;

public:
    
    line2_uint16()
    {
        A = {};
        B = {};
    }

    line2_uint16(vec2_uint16 a, vec2_uint16 b)
    {
        A = a; B = b;
    }

    // TODO Copy Constructor

};

struct quad2_uint16
{
    vec2_uint16 A;
    vec2_uint16 B;
    vec2_uint16 C;
    vec2_uint16 D;

public: 
    quad2_uint16()
    {
        A = {};
        B = {};
        C = {};
        D = {};
    }

    quad2_uint16(vec2_uint16 a, vec2_uint16 b, vec2_uint16 c, vec2_uint16 d)
    {
        A = a; B = b; C = c, D = d;
    }
    
    // TODO Copy constructor

};

// Forward declares
void draw_line(Uint32* pixels, int x0, int y0, int x1, int y1, int color);
void draw_line(Uint32* pixels, line2_uint16& line, int color);
void draw_quad_fill(Uint32* pixels, quad2_uint16& quad, int color);
void draw_quad_wireframe(Uint32* pixels, quad2_uint16& quad, int color);

void draw_quad_segment(Uint32* pixels, line2_uint16& left, line2_uint16& right);
void draw_line_horizontal(Uint32* pixels, Uint16 x0, Uint16 y0, Uint16 length, int color);


struct vertex3f
{
    float X;
    float Y;
    float Z;
};

struct vertex2f_Projected
{
    float X;
    float Y;
    float D;

public:
    vertex2f_Projected(float x, float y)
    {
        X = x;
        Y = y;
        D = 0;
    }
    vertex2f_Projected(float x, float y, float d)
    {
        X = x;
        Y = y;
        D = d;
    }
};

void ndc_to_ScreenSpace(vertex2f_Projected& out)
{
    float x_proj_remap = (1 + out.X) / 2;       // Maps between 0-1
    float y_proj_remap = (1 + out.Y) / 2;


    out.X = x_proj_remap * INTERNAL_WIDTH;
    out.Y = y_proj_remap * INTERNAL_HEIGHT;
}


vertex2f_Projected viewspace_to_ndc(vertex3f& in)
{
    // Using FOV of 90 degrees.
    float x_proj = -1 * in.X / in.Z;
    float y_proj = -1 * in.Y / in.Z;

    return vertex2f_Projected(x_proj, y_proj, in.Z);
}

struct model_cube
{
    // World space X,Y Z=height
    vertex3f corners[12] = 
    {
        { 4, -1, 2},
        { 4, 1, 2},
        { 4, -1, -1},
        { 4,  1, -1},

        { 4, 1, 2},
        { 4, 2, 1},
        { 4, 1, -1},
        { 4, 2, -1},

        { 4, -2, 0.5},
        { 4, -1, 1},
        { 4, -2, -1},
        { 4, -1, -1},
    };
};

model_cube model;

vertex3f vertexbuffer[12] = { };

void render() 
{
    if (SDL_MUSTLOCK(surface)) 
    {
        SDL_LockSurface(surface);
    }

    Uint32 * pixels = (Uint32*) surface->pixels;
    
    SDL_Rect rect = SDL_Rect();
    rect.w = INTERNAL_WIDTH;
    rect.h = INTERNAL_HEIGHT;
    SDL_FillRect(surface, &rect, 0);

    // Draw the pixels
    {

        // world space to camera(view) space.
        int len = sizeof(model.corners) / sizeof(model.corners[0]);
        for (int i = 0; i < len; i++)
        {
            vertexbuffer[i] = model.corners[i];
            float x = -1 * model.corners[i].Y;
            float y = model.corners[i].X;
            float z = model.corners[i].Z;

            if (true)
            {
                // DO I need a proper matrix for this maybe it's not good enough...

                // I think I need a proper worldToCamera tranformation matrix...

                // WorldSpace:
                //      Cartesion from top down with Z being the height
                // Camera space:
                //      with -Z being the axis the camera is looking down. +X to the right and +Y to the top

                float distance = sqrt(x * x + y * y);
                                
                // Translate wrt the camera position
                y -= cameraZoom;
                
                // Rotate wrt the camera rotation
                float rads = cameraRotationRadians;
           
                // Does X and Y always need to be rotated by 90 first?
                float tx = cos(rads) * x + sin(rads) * y;
                float ty = sin(rads) * x - cos(rads) * y;

                // Now we are in camera(view) space
                vertexbuffer[i].X = -1 * tx / distance;
                vertexbuffer[i].Y = -1 * z / distance;               // Becomes the height
                vertexbuffer[i].Z = ty / distance;
            }
        }
        


        // TODO Colate into quads for rendering...

        // Quad A
        {
            // Project the first 4 points
            auto projectedA = viewspace_to_ndc(vertexbuffer[0]);
            auto projectedB = viewspace_to_ndc(vertexbuffer[1]);
            auto projectedC = viewspace_to_ndc(vertexbuffer[3]);
            auto projectedD = viewspace_to_ndc(vertexbuffer[2]);


            // TODO Solve for Clipspace

            // For pixel coords
            ndc_to_ScreenSpace(projectedA);
            ndc_to_ScreenSpace(projectedB);
            ndc_to_ScreenSpace(projectedC);
            ndc_to_ScreenSpace(projectedD);

            quad2_uint16 quadA = quad2_uint16(vec2_uint16(projectedA.X, projectedA.Y), vec2_uint16(projectedB.X, projectedB.Y), vec2_uint16(projectedC.X, projectedC.Y), vec2_uint16(projectedD.X, projectedD.Y));

            draw_quad_fill(pixels, quadA, 255);
            draw_quad_wireframe(pixels, quadA, 255);
        }

        // Quad B
        {
            auto projectedA = viewspace_to_ndc(vertexbuffer[4]);
            auto projectedB = viewspace_to_ndc(vertexbuffer[5]);
            auto projectedC = viewspace_to_ndc(vertexbuffer[7]);
            auto projectedD = viewspace_to_ndc(vertexbuffer[6]);

            // TODO Solve for Clipspace
            
            // For pixel coords
            ndc_to_ScreenSpace(projectedA);
            ndc_to_ScreenSpace(projectedB);
            ndc_to_ScreenSpace(projectedC);
            ndc_to_ScreenSpace(projectedD);

            quad2_uint16 quadA = quad2_uint16(vec2_uint16(projectedA.X, projectedA.Y), vec2_uint16(projectedB.X, projectedB.Y), vec2_uint16(projectedC.X, projectedC.Y), vec2_uint16(projectedD.X, projectedD.Y));

            //draw_quad_fill(pixels, quadA, 255);
            draw_quad_wireframe(pixels, quadA, 255);
        }

        // Quad C
        {
            auto projectedA = viewspace_to_ndc(vertexbuffer[8]);
            auto projectedB = viewspace_to_ndc(vertexbuffer[9]);
            auto projectedC = viewspace_to_ndc(vertexbuffer[11]);
            auto projectedD = viewspace_to_ndc(vertexbuffer[10]);

            // TODO Solve for Clipspace

            // For pixel coords
            ndc_to_ScreenSpace(projectedA);
            ndc_to_ScreenSpace(projectedB);
            ndc_to_ScreenSpace(projectedC);
            ndc_to_ScreenSpace(projectedD);

            quad2_uint16 quadA = quad2_uint16(vec2_uint16(projectedA.X, projectedA.Y), vec2_uint16(projectedB.X, projectedB.Y), vec2_uint16(projectedC.X, projectedC.Y), vec2_uint16(projectedD.X, projectedD.Y));

            //draw_quad_fill//(pixels, quadA, 255);
            draw_quad_wireframe(pixels, quadA, 255);
        }
    }

    if (SDL_MUSTLOCK(surface))
    {
        SDL_UnlockSurface(surface);
    }

    SDL_Texture *screenTexture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
    SDL_RenderPresent(renderer);

    SDL_DestroyTexture(screenTexture);
}


void swap(int& a, int& b)
{
    a = a xor b;
    b = a xor b;
    a = a xor b;
}

void swap(vec2_uint16& a, vec2_uint16& b)
{
    vec2_uint16 tmp = b;
    b = a;
    a = tmp;
}

// Try to limit to internal use so can be properly clamped
void setPixel(Uint32* pixels, int x, int y, int color)
{
    int index = y * INTERNAL_WIDTH + x;
    pixels[index] = color;
}

int clamp(int x0, int min, int max)
{
    if (x0 > max)
    {
        return max;
    }
    if (x0 < min)
    {
        return min;
    }
    return x0;
}

Uint16 min(Uint16 a, Uint16 b)
{
    if (a < b) return a;
    return b;
}

void draw_quad_wireframe(Uint32* pixels, quad2_uint16& quad, int color)
{

    line2_uint16 line_a = line2_uint16(quad.A, quad.B);
    line2_uint16 line_b = line2_uint16(quad.B, quad.C);
    line2_uint16 line_c = line2_uint16(quad.C, quad.D);
    line2_uint16 line_d = line2_uint16(quad.D, quad.A);

    // Draw wireframe
    {
        draw_line(pixels, line_a.A.X, line_a.A.Y, line_a.B.X, line_a.B.Y, color * 255);
        draw_line(pixels, line_b.A.X, line_b.A.Y, line_b.B.X, line_b.B.Y, color * 255);
        draw_line(pixels, line_c.A.X, line_c.A.Y, line_c.B.X, line_c.B.Y, color * 255);
        draw_line(pixels, line_d.A.X, line_d.A.Y, line_d.B.X, line_d.B.Y, color * 255);
    }
}

void draw_quad_fill(Uint32* pixels, quad2_uint16& quad, int color)
{
    // Draw fill

    // TODO Fix the fill

    // Sort the 4 verticies from top to bottom

    // Want to find verticie at the top of the screen.
    // This then determines the clockwise order as the input is already clockwise
 
    line2_uint16 line_a = line2_uint16(quad.A, quad.B);
    line2_uint16 line_b = line2_uint16(quad.B, quad.C);
    line2_uint16 line_c = line2_uint16(quad.C, quad.D);
    line2_uint16 line_d = line2_uint16(quad.D, quad.A);


    // Find line segment with highest Y value and reorder.

    // Run sort 4 and split into 3 buckets with left and right vectors for sides

    // determine the scanlines required by solving for intersection of horizontal line against the left and right vectors of the bucket

    // draw horizontal line for each of the 3 buckets.

    // TODO Be a bit smarter about choosing these (see above)
    line2_uint16 leftA = line2_uint16(quad.C, quad.B);
    line2_uint16 rightA = line2_uint16(quad.C, quad.D);
    draw_quad_segment(pixels, leftA, rightA);

    line2_uint16 leftB = line2_uint16(quad.B, quad.A);
    line2_uint16 rightB = line2_uint16(quad.C, quad.D);
    draw_quad_segment(pixels, leftB, rightB);

    line2_uint16 leftC = line2_uint16(quad.A, quad.D);
    line2_uint16 rightC = line2_uint16(quad.C, quad.D);
    draw_quad_segment(pixels, leftC, rightC);
}


/// <summary>
/// Draws a trapizoid shape by defining a left line segment and a right line segment and drawing horizontal lines between them in a scanline algorithm line fashion.
/// </summary>
void draw_quad_segment(Uint32* pixels, line2_uint16& left, line2_uint16& right)
{
    // Just temp hand code the drawing of the first segment
    {
        
        float dX_left = (left.B.X - left.A.X);
        float dX_right = (right.B.X - right.A.X);

        Uint16 dY_right = std::abs(right.B.Y - right.A.Y);
        Uint16 dY_left = std::abs(left.B.Y - left.A.Y);

        // TODO Can these cases etc... be simplified? Maybe into somekind of DX_L and DX_R factor? OR is keeping them in these cases actually better/faster?

        if (dX_left != 0 && dX_right != 0)
        {
            float gradLeft = -1 * dY_left / (float)dX_left;
            float gradRight = -1 * dY_right / (float)dX_right;

            // Draw from bottom to top
            for (int i = left.B.Y; i > left.A.Y; i--)
            {
                // Now need to know lprime and rprime to find the width and starting point of the line
                int l_prime = left.B.X - (i - left.B.Y) / gradLeft;
                int r_prime = right.B.X - (i - right.B.Y) / gradRight;

                int width = (r_prime - l_prime) + 1;
                draw_line_horizontal(pixels, l_prime, i, width, 255);
            }
        }
        else
        {
            if (dX_left == 0 && dX_right == 0)
            {
                float gradRight = -1 * dY_right / (float)dX_right;
                int l_prime = left.B.X;
                int r_prime = right.B.X;

                for (int i = left.B.Y; i > left.A.Y; i--)
                {
                    int width = (r_prime - l_prime) + 1;
                    draw_line_horizontal(pixels, l_prime, i, width, 255);
                }
            }
            else
            {
                if (dX_left == 0)
                {
                    float gradRight = -1 * dY_right / (float)dX_right;
                    int l_prime = left.B.X;

                    for (int i = left.B.Y; i > left.A.Y; i--)
                    {
                        int r_prime = right.B.X - (i - right.B.Y) / gradRight;

                        int width = (r_prime - l_prime) + 1;
                        draw_line_horizontal(pixels, l_prime, i, width, 255);
                    }
                }
                else
                {
                    float gradLeft = -1 * dY_left / (float)dX_left;
                    int r_prime = right.B.X;

                    // Draw from bottom to top
                    for (int i = left.B.Y; i > left.A.Y; i--)
                    {
                        // Now need to know lprime and rprime to find the width and starting point of the line
                        int l_prime = left.B.X - (i - left.B.Y) / gradLeft;
                      
                        int width = (r_prime - l_prime) + 1;
                        draw_line_horizontal(pixels, l_prime, i, width, 255);
                    }
                }
            }
        }
    }
}

void draw_line_horizontal(Uint32* pixels, Uint16 x0, Uint16 y0, Uint16 length, int color)
{
    int x = x0;
    int y = y0;
    if (x + length > INTERNAL_WIDTH - 1)
    {
        length = INTERNAL_WIDTH - x;
    }

    for (int i = 0; i < length; i++)
    {
        setPixel(pixels, x+i, y, color);
    }
}

void draw_line(Uint32* pixels, line2_uint16& line, int color)
{
    draw_line(pixels, line.A.X, line.A.Y, line.B.X, line.B.Y, color);
}

void draw_line(Uint32* pixels, int x0, int y0, int x1, int y1, int color)
{
    // Breshams line drawing algoritm
    // 
    // clamp input
    x0 = clamp(x0, 0, INTERNAL_WIDTH - 1);
    y0 = clamp(y0, 0, INTERNAL_HEIGHT - 1);

    x1 = clamp(x1, 0, INTERNAL_WIDTH - 1);
    y1 = clamp(y1, 0, INTERNAL_HEIGHT - 1);
    
    // Flip y coordinate;
    //y0 = INTERNAL_HEIGHT - y0;
    //y1 = INTERNAL_HEIGHT - y1;

    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) 
    {
        swap(x0, y0);
        swap(x1, y1);
        steep = true;
    }
    
    if (x0 > x1) 
    {
        swap(x0, x1);
        swap(y0, y1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;
    int derror2 = std::abs(dy) * 2;
    int error2 = 0;
    int y = y0;
    
    if (steep)
    {
        for (int x = x0; x <= x1; x++)
        {
            setPixel(pixels, y, x, color);

            error2 += derror2;

            if (error2 > dx)
            {
                y += (y1 > y0 ? 1 : -1);
                error2 -= dx * 2;
            }
        }
    }
    else
    {
        for (int x = x0; x <= x1; x++)
        {
            setPixel(pixels, x, y, color);

            error2 += derror2;
            if (error2 > dx)
            {
                y += (y1 > y0 ? 1 : -1);
                error2 -= dx * 2;
            }
        }
    }
}



void windows_main()
{
    //Main loop flag
    bool quit = false;

    //Event handler
    SDL_Event e;
    
    int number = 33;
    char buffer[16];

    // TODO Need a time.delta time
    //While application is running
    while (!quit)
    {
        SDL_SetWindowTitle(window, buffer);

        //Handle events on queue
        while (SDL_PollEvent(&e) != 0)
        {
            //User requests quit
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }

            if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_LEFT)
                {
                    cameraRotationRadians += 0.01;
                    if (cameraRotationRadians > M_2PI)
                    {
                        cameraRotationRadians -= M_2PI;
                    }
                } 
                else if (e.key.keysym.sym == SDLK_RIGHT)
                {
                    cameraRotationRadians -= 0.01;
                    if (cameraRotationRadians < 0)
                    {
                        cameraRotationRadians += M_2PI;
                    }
                }
                else if (e.key.keysym.sym == SDLK_a)
                {
                    // TODO Put these values on the camera struct
                    cameraZoom += 0.2;
                    if (cameraZoom > 16)
                    {
                        cameraZoom = 16;
                    }
                }
                else if (e.key.keysym.sym == SDLK_z)
                {
                    // TODO Put these values on the camera struct
                    cameraZoom -= 0.2;
                    if (cameraZoom < -16)
                    {
                        cameraZoom = -16;
                    }
                }
            }
        }

        render();

        fps_frames++;
        if (fps_lasttime < SDL_GetTicks() - FPS_INTERVAL * 1000)
        {
            fps_lasttime = SDL_GetTicks();
            fps_current = fps_frames;
            fps_frames = 0;

            sprintf_s(buffer, "%d", fps_current);
        }
    }
}

int main(int argc, char* argv[]) 
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(INTERNAL_WIDTH*PIXEL_SCALE, INTERNAL_HEIGHT*PIXEL_SCALE, 0, &window, &renderer);
    
    surface = SDL_CreateRGBSurface(0, INTERNAL_WIDTH, INTERNAL_HEIGHT, 32, 0, 0, 0, 0);
    
    #ifdef __EMSCRIPTEN__
        emscripten_set_main_loop(render, 0, 1);
    #else
        windows_main();
    #endif
}

