#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

typedef int8_t      s8;
typedef int16_t     s16;
typedef int32_t     s32;
typedef int64_t     s64;

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef float       f32;
typedef double      f64;

typedef bool        b32;

typedef struct key_state
{
    u32 transitions;
    b32 key_down;
} key_state;

typedef struct game_input
{
    f32 delta_time;

    union 
    {
        key_state keys[7];

        struct
        {
            key_state move_up;
            key_state move_down;
            key_state move_right;
            key_state move_left;

            key_state reload;
            key_state shoot;

            key_state back;
        };
    };

    s32 mouse_x;
    s32 mouse_y;
} game_input;

typedef struct game_memory
{
    b32 initialized;
    void* base;
    u64 size;
} game_memory;

void _log(s8* format, ...)
{
    // Todo: implement own vfprintf function
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

#define LOG(format, ...) _log(##format, __VA_ARGS__);