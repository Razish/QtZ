#pragma once

#include "tr_ext_glsl_program.h"
#include "tr_ext_framebuffer.h"

void R_EXT_Init( void );
void R_EXT_Cleanup( void );
void R_EXT_PostProcess( void );
void DrawQuad( float x, float y, float width, float height );

#define NUM_LUMINANCE_FBOS (9)
#define LUMINANCE_FBO_SIZE (256.0f)
#define NUM_GLOW_DOWNSCALE_FBOS (3)
#define NUM_BLOOM_DOWNSCALE_FBOS (8)
#define GLOW_DOWNSCALE_RATE (2.0f)
#define BLOOM_DOWNSCALE_RATE (2.0f)
