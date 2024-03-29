#ifndef LIGER_BUILTIN_COMMON_H
#define LIGER_BUILTIN_COMMON_H

/************************************************************************************************
 * Constants
 ************************************************************************************************/
const float PI      = 3.14159265359;
const vec3  GRAVITY = vec3(0, -9.8, 0);

const vec2 SCREEN_QUAD_POSITIONS[6] = {
    // Lower triangle
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),

    // Upper triangle
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0),
};

/************************************************************************************************
 * Functions
 ************************************************************************************************/
float RandomFloat(float co) { return fract(sin(co*(91.3458)) * 47453.5453); }

#endif  // LIGER_BUILTIN_COMMON_H