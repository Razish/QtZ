#include "GLee.h"
#include "tr_local.h"
#include "tr_ext_public.h"

static const float colourWhite[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

// ================
//
// FBO/Shader helper functions
//
// ================

static framebuffer_t *R_EXT_CreateFBO( unsigned int width, unsigned int height, internalFormat_t colorbits, internalFormat_t depthbits )
{//Helper function to create an FBO
	framebuffer_t *fbo = R_EXT_CreateFramebuffer();
	R_EXT_BindFramebuffer( fbo );

	//Create color/depth attachments if necessary
	if ( colorbits )
		R_EXT_AttachColorTextureToFramebuffer( fbo, R_EXT_CreateBlankTexture( width, height, colorbits ), 0 );
	if ( depthbits )
		R_EXT_AttachDepthStencilTextureToFramebuffer( fbo, R_EXT_CreateTexture( width, height, depthbits, 1, 1 ) );

	//Check for errors
	R_EXT_CheckFramebuffer( fbo );

	return fbo;
}

static glslProgram_t *R_EXT_CreateShader( const char *source )
{//Helper function to create a GLSL ##FRAGMENT## shader
	glslProgram_t *shader = R_EXT_GLSL_CreateProgram();

	R_EXT_GLSL_AttachShader( shader, R_EXT_GLSL_CreateFragmentShader( source ) );
	R_EXT_GLSL_LinkProgram( shader );
	R_EXT_GLSL_UseProgram( shader );

	return shader;
}

//Uses a static buffer, do NOT nest calls
static const char *ShaderSource( const char *path )
{
	const char *contents = NULL;
	static char buf[1024*64] = {0};

	ri->FS_ReadFile( path, (void **)&contents );

	if ( !contents )
	{
		ri->Error( ERR_DROP, "Couldn't load GLSL shader %s\n", path );
		tr.postprocessing.loaded = qfalse;
		return NULL;
	}

	else
	{
		ri->Printf( PRINT_DEVELOPER, "Loading GLSL shader %s\n", path );
		Q_strncpyz( buf, contents, sizeof( buf ) );
		COM_Compress( buf );
		ri->FS_FreeFile( (void *)contents );
		return &buf[0];
	}
}

// ================
//
// FBO/Shader initialisation
//
// ================

static void CreateFramebuffers( void )
{
	float	w = (float)glConfig.vidWidth, h = (float)glConfig.vidHeight;
	float	size = LUMINANCE_FBO_SIZE;
	int i;
	int steps = Q_clampi( 1, r_postprocess_bloomSteps->integer, NUM_BLOOM_DOWNSCALE_FBOS );

	tr.postprocessing.bloomSize = r_postprocess_bloomSize->integer ? r_postprocess_bloomSize->value : w;
	tr.postprocessing.glowSize = r_postprocess_glowSize->integer ? r_postprocess_glowSize->value : w;
	tr.postprocessing.bloomSteps = steps;

	//scene
	tr.framebuffers.scene					= R_EXT_CreateFBO( (int)w, (int)h, IF_RGBA16F, IF_DEPTH24_STENCIL8 );
//	R_EXT_AttachDepthStencilTextureToFramebuffer( tr.framebuffers.scene, R_EXT_CreateTexture( w, h, IF_DEPTH24_STENCIL8, 1, 1 ) );

	//dynamic glow
	tr.framebuffers.glow					= R_EXT_CreateFBO( (int)w, (int)h, IF_RGBA16F, IF_DEPTH24_STENCIL8 );
	R_EXT_AttachDepthStencilTextureToFramebuffer( tr.framebuffers.glow, tr.framebuffers.scene->depthTexture );
//	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_STENCIL_ATTACHMENT, TT_POT, tr.framebuffers.scene->depthTexture, 0 );

	for ( i=0, size=1.0f; i<NUM_GLOW_DOWNSCALE_FBOS; i++, size *= GLOW_DOWNSCALE_RATE )
		tr.framebuffers.glowDownscale[i]	= R_EXT_CreateFBO( (int)floorf( tr.postprocessing.glowSize/size ), (int)floorf( tr.postprocessing.glowSize/size ), IF_RGBA8, IF_NONE );
	for ( i=0; i<2; i++ )
		tr.framebuffers.glowBlur[i]			= R_EXT_CreateFBO( (int)floorf( tr.postprocessing.glowSize/size ), (int)floorf( tr.postprocessing.glowSize/size ), IF_RGBA8, IF_NONE );
	tr.framebuffers.glowComposite			= R_EXT_CreateFBO( (int)w, (int)h, IF_RGBA8, IF_NONE );

	//hdr tonemapping
	for ( i=0, size=LUMINANCE_FBO_SIZE; i<NUM_LUMINANCE_FBOS; i++, size /= 2.0f )
		tr.framebuffers.luminance[i]		= R_EXT_CreateFBO( (int)floorf(size), (int)floorf(size), IF_RGBA8, IF_NONE );

	tr.framebuffers.tonemapping				= R_EXT_CreateFBO( (int)w, (int)h, IF_RGBA8, IF_NONE );
	tr.framebuffers.brightPass				= R_EXT_CreateFBO( (int)w, (int)h, IF_RGBA8, IF_NONE );

	//bloom
	for ( i=0, size=1.0f; i<tr.postprocessing.bloomSteps; i++, size *= BLOOM_DOWNSCALE_RATE )
		tr.framebuffers.bloomDownscale[i]	= R_EXT_CreateFBO( (int)floorf( tr.postprocessing.bloomSize/size ), (int)floorf( tr.postprocessing.bloomSize/size ), IF_RGBA8, IF_NONE );
	for ( i=0; i<2; i++ )
		tr.framebuffers.bloomBlur[i]		= R_EXT_CreateFBO( (int)floorf( tr.postprocessing.bloomSize/size ), (int)floorf( tr.postprocessing.bloomSize/size ), IF_RGBA8, IF_NONE );
	tr.framebuffers.bloom					= R_EXT_CreateFBO( (int)w, (int)h, IF_RGBA8, IF_NONE );

	//final pass
	tr.framebuffers.final					= R_EXT_CreateFBO( (int)w, (int)h, IF_RGBA8, IF_NONE );
}

static void LoadShaders( void )
{
	// Dynamic glow
	tr.glsl.glow = R_EXT_CreateShader( ShaderSource( "shaders/post/glow.glsl" ) );

	// HDR Tonemapping
	tr.glsl.tonemapping = R_EXT_CreateShader( ShaderSource( "shaders/post/tonemapping.glsl" ) );
	R_EXT_GLSL_SetUniform1i( tr.glsl.tonemapping, "SceneTex", 0 );

	// Luminance
	tr.glsl.luminance = R_EXT_CreateShader( ShaderSource( "shaders/post/luminance.glsl" ) );
	R_EXT_GLSL_SetUniform1i( tr.glsl.luminance, "SceneTex", 0 );
	R_EXT_GLSL_SetUniform1f( tr.glsl.luminance, "PixelWidth", (1.0f / LUMINANCE_FBO_SIZE)*0.5f );
	R_EXT_GLSL_SetUniform1f( tr.glsl.luminance, "PixelHeight", (1.0f / LUMINANCE_FBO_SIZE)*0.5f );

	// Downscale luminance
	tr.glsl.downscaleLuminance = R_EXT_CreateShader( ShaderSource( "shaders/post/downscaleLuminance.glsl" ) );
	R_EXT_GLSL_SetUniform1i( tr.glsl.downscaleLuminance, "SceneTex", 0 );

	// Bright pass filter
	tr.glsl.brightPass = R_EXT_CreateShader( ShaderSource( "shaders/post/brightpass.glsl" ) );
	R_EXT_GLSL_SetUniform1i( tr.glsl.brightPass, "SceneTex", 0 );

	// Gaussian blurs
	tr.glsl.gaussianBlur3x3[0] = R_EXT_CreateShader( ShaderSource( "shaders/post/gaussianBlur3x3_horizontal.glsl" ) );
	R_EXT_GLSL_SetUniform1i( tr.glsl.gaussianBlur3x3[0], "SceneTex", 0 );
	tr.glsl.gaussianBlur3x3[1] = R_EXT_CreateShader( ShaderSource( "shaders/post/gaussianBlur3x3_vertical.glsl" ) );
	R_EXT_GLSL_SetUniform1i( tr.glsl.gaussianBlur3x3[1], "SceneTex", 0 );

	tr.glsl.gaussianBlur6x6[0] = R_EXT_CreateShader( ShaderSource( "shaders/post/gaussianBlur6x6_horizontal.glsl" ) );
	R_EXT_GLSL_SetUniform1i( tr.glsl.gaussianBlur6x6[0], "SceneTex", 0 );
	tr.glsl.gaussianBlur6x6[1] = R_EXT_CreateShader( ShaderSource( "shaders/post/gaussianBlur6x6_vertical.glsl" ) );
	R_EXT_GLSL_SetUniform1i( tr.glsl.gaussianBlur6x6[1], "SceneTex", 0 );

	// bloom
	tr.glsl.bloom = R_EXT_CreateShader( ShaderSource( "shaders/post/bloom.glsl" ) );

	// final pass, colour correction and such
	tr.glsl.final = R_EXT_CreateShader( ShaderSource( "shaders/post/final.glsl" ) );
	R_EXT_GLSL_SetUniform1i( tr.glsl.final, "SceneTex", 0 );
}

void R_EXT_Init( void )
{
	ri->Printf( PRINT_ALL, "----------------------------\n" );
	Com_Printf( "Loading visual extensions...\n" );

	tr.postprocessing.loaded = qfalse;
	if ( !R_EXT_GLSL_Init() || !R_EXT_FramebufferInit() )
		return;
	tr.postprocessing.loaded = qtrue;

	ri->Printf( PRINT_ALL, "----------------------------\n" );

	CreateFramebuffers();
	LoadShaders();

	R_EXT_GLSL_UseProgram( NULL );
	R_EXT_BindDefaultFramebuffer();
}

// ================
//
// Shutdown
//
// ================

void R_EXT_Cleanup( void )
{
//	glColor4fv( colourWhite ); //HACK: Sometimes scopes change the colour.
	R_SyncRenderThread();

	R_EXT_FramebufferCleanup();
	R_EXT_GLSL_Cleanup();
	tr.postprocessing.loaded = qfalse;
}

// ================
//
// Post processing
//
// ================

void SCR_AdjustFrom640( float *x, float *y, float *w, float *h ) {
	float	xscale;
	float	yscale;

	// scale for screen sizes
	xscale = glConfig.vidWidth / SCREEN_WIDTH;
	yscale = glConfig.vidHeight / SCREEN_HEIGHT;
	if ( x )	*x *= xscale;
	if ( y )	*y *= yscale;
	if ( w )	*w *= xscale;
	if ( h )	*h *= yscale;
}

void DrawQuad( float x, float y, float width, float height )
{
	float _x=x, _y=y, _width=width, _height=height;
	SCR_AdjustFrom640( &_x, &_y, &_width, &_height );
	glBegin( GL_QUADS );
		glTexCoord2f (0.0f, 0.0f);
		glVertex2f (_x, _y + _height);

		glTexCoord2f (0.0f, 1.0f);
		glVertex2f (_x, _y);

		glTexCoord2f (1.0f, 1.0f);
		glVertex2f (_x + _width, _y);

		glTexCoord2f (1.0f, 0.0f);
		glVertex2f (_x + _width, _y + _height);
	glEnd();
}

#define DrawFullscreenQuad() DrawQuad( 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT )

//Light adaptation
static float adaptedFrameLuminance	= 0.0f;
static float adaptedMaxLuminance	= 1.0f;
static float lastFrameTime			= 0.0f;
static void CalculateLightAdaptation( void )
{
	float	time = ri->Milliseconds() / 1000.0f;
	float	dt;
	vector4	currentFrameLuminance = { 0.0f };
	float tau = 1.0f;
		float rods = 0.4f, cones = 0.8f;

	R_EXT_BindFramebuffer( tr.framebuffers.luminance[NUM_LUMINANCE_FBOS - 1] );
	glReadPixels( 0, 0, 1, 1, GL_RGBA, GL_FLOAT, &currentFrameLuminance );

	tau = (currentFrameLuminance.x * rods) + ((1.0f-currentFrameLuminance.x) * cones);

	dt = MAX( time-lastFrameTime, 0.0f );

	//adaptedFrameLuminance = adaptedFrameLuminance + (currentFrameLuminance[0] - adaptedFrameLuminance) * (1.0f - powf (0.98f, 30.0f * dt));
	adaptedFrameLuminance	+= (currentFrameLuminance.y - adaptedFrameLuminance)	* (1.0f - expf(-dt * tau));
	adaptedMaxLuminance		+= (currentFrameLuminance.z - adaptedMaxLuminance)		* (1.0f - expf(-dt * tau));
	lastFrameTime = time;
}

static QINLINE void ResizeTarget( int w, int h ) {
	glViewport( 0, 0, w, h );
	glScissor( 0, 0, w, h );
}

void RB_PostProcess( void )
{
	int i;
	float w = (float)glConfig.vidWidth, h = (float)glConfig.vidHeight;
	framebuffer_t *lastFullscreenFBO = tr.framebuffers.scene;

	//QTZFIXME: this may be bad because we render into an off-screen buffer even if we're not doing post processing
	if ( !r_postprocess_enable->integer || (!tr.postprocessing.loaded) )
		return;

    RB_EndSurface();
    RB_SetGL2D();
//	RE_StretchPic( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0 );
//	R_SyncRenderThread();
	qglColor4fv( colourWhite ); //HACK: The colour likes to be weird values ._.
	
//	R_EXT_BlitFramebufferColorAndDepth( NULL, tr.framebuffers.scene, w, h, w, h );

	// *	logical step
	// -->	"off-screen" render operation
	// <<<	output render operation
	// >>>	input FBO


	//Dynamic Glow
	//	* 3x3 gaussian blur glow surfaces
	//--> render into BLUR FBOs
	//	* additively blend scene + glow
	//<<< SCENE FBO
	//>>> render into BLUR COMPOSITE FBO

	//High Dynamic Range Tonemapping
	//	* calculate scene luminance
	//--> render into DOWNSCALE LUMINANCE FBOs
	//--> render into LUMINANCE FBO
	//	* simulate eye adaptation
	//	* tonemap the scene
	//<<< BLUR COMPOSITE FBO
	//>>> render into TONEMAPPING FBO

	//Bloom
	//	* brightpass filter to separate bright pixels
	//--> render into BRIGHTPASS FBO
	//	* downscale texture with bright pixels
	//--> render into DOWNSCALE FBOs
	//	* 6x6 gaussian blur bloom pixels
	//--> render into BLUR FBOs
	//	* upscale texture
	//--> render into BRIGHTPASS FBO
	//	* additively blend scene + bloom
	//<<< TONEMAPPING FBO
	//>>> render into BLOOM FBO

	//Final Pass
	//	* colour filter for visibility
	//<<< BLOOM FBO
	//>>> FINALPASS FBO

	//Display
	//<<< FINALPASS FBO
	//>>> BACKBUFFER


	//================================
	//	Dynamic Glow
	//================================

	if ( r_postprocess_glow->integer )
	{
		framebuffer_t *lastFBO = tr.framebuffers.glow;
		glslProgram_t *shader = NULL;
		float size = 1.0f;

		qglPushAttrib( GL_VIEWPORT_BIT|GL_SCISSOR_BIT );

		R_EXT_GLSL_UseProgram( NULL );
		for ( i=0, size=1.0f; i<NUM_GLOW_DOWNSCALE_FBOS; i++, size *= GLOW_DOWNSCALE_RATE )
		{// downscale
			R_EXT_BindFramebuffer( tr.framebuffers.glowDownscale[i] );
			GL_Bind( lastFBO->colorTextures[0]->q3image );
			ResizeTarget( floorf( tr.postprocessing.glowSize/size ), floorf( tr.postprocessing.glowSize/size ) );
			DrawFullscreenQuad();
			lastFBO = tr.framebuffers.glowDownscale[i];
		}

		for ( i=0; i<2; i++ )
		{// blur
			if ( !(i & 1) && r_postprocess_glowBlurStyle->integer == 2 )
				shader = tr.glsl.gaussianBlur6x6[i];
			else
				shader = tr.glsl.gaussianBlur3x3[i];
			R_EXT_BindFramebuffer( tr.framebuffers.glowBlur[i] );
			R_EXT_GLSL_UseProgram( shader );
			if ( !(i & 1) )	R_EXT_GLSL_SetUniform1f( shader, "PixelWidth", 1.0f/floorf(tr.postprocessing.glowSize/size) );
			else			R_EXT_GLSL_SetUniform1f( shader, "PixelHeight", 1.0f/floorf(tr.postprocessing.glowSize/size) );
			GL_Bind( lastFBO->colorTextures[0]->q3image );
			if ( !(i & 1) ) ResizeTarget( floorf( tr.postprocessing.glowSize/size ), floorf( tr.postprocessing.glowSize/size ) );
			DrawFullscreenQuad();
			lastFBO = tr.framebuffers.glowBlur[i];
		}

		R_EXT_GLSL_UseProgram( NULL ); //reset the active shader
		for ( i=NUM_GLOW_DOWNSCALE_FBOS-1, size/=GLOW_DOWNSCALE_RATE; i>=0; i--, size /= GLOW_DOWNSCALE_RATE )
		{// upscale
			R_EXT_BindFramebuffer( tr.framebuffers.glowDownscale[i] );
			GL_Bind( lastFBO->colorTextures[0]->q3image );
			ResizeTarget( floorf( tr.postprocessing.glowSize/size ), floorf( tr.postprocessing.glowSize/size ) );
			DrawFullscreenQuad();
			lastFBO = tr.framebuffers.glowDownscale[i];
		}

		//finally upscale into the glow fbo (full screen size)
		R_EXT_BindFramebuffer( tr.framebuffers.glow );
		GL_Bind( lastFBO->colorTextures[0]->q3image );
		ResizeTarget( w, h );
		DrawFullscreenQuad();
		lastFBO = tr.framebuffers.glow;
		qglPopAttrib();

		// add the glow texture onto the scene texture
		R_EXT_BindFramebuffer( tr.framebuffers.glowComposite );
		R_EXT_GLSL_UseProgram( tr.glsl.glow );
		GL_SelectTexture( 0 );	GL_Bind( lastFullscreenFBO->colorTextures[0]->q3image );	R_EXT_GLSL_SetUniform1i( tr.glsl.glow, "SceneTex", 0 );
		GL_SelectTexture( 1 );	GL_Bind( lastFBO->colorTextures[0]->q3image );				R_EXT_GLSL_SetUniform1i( tr.glsl.glow, "GlowTex", 1 );
		R_EXT_GLSL_SetUniform1f( tr.glsl.glow, "GlowMulti", r_postprocess_glowMulti->value );
		R_EXT_GLSL_SetUniform1f( tr.glsl.glow, "GlowSat", r_postprocess_glowSat->value );
		DrawFullscreenQuad();
		lastFullscreenFBO = tr.framebuffers.glowComposite;
		GL_SelectTexture( 0 );
	}

	//================================
	//	High Dynamic Range
	//================================

	if ( r_postprocess_hdr->integer )
	{
		int size = tr.framebuffers.luminance[0]->colorTextures[0]->width;
		qglPushAttrib( GL_VIEWPORT_BIT|GL_SCISSOR_BIT );
		ResizeTarget( size, size );

		// Downscale and convert to luminance.    
		R_EXT_GLSL_UseProgram( tr.glsl.luminance );
		R_EXT_BindFramebuffer( tr.framebuffers.luminance[0] );
		GL_Bind( lastFullscreenFBO->colorTextures[0]->q3image );
		DrawFullscreenQuad();

        R_EXT_BlitFramebufferColor( tr.framebuffers.luminance[0], tr.framebuffers.tonemapping, (int)w, (int)h, (int)w, (int)h );

		// Downscale further to 1x1 texture.
		R_EXT_GLSL_UseProgram( tr.glsl.downscaleLuminance );
		for ( i=1; i<NUM_LUMINANCE_FBOS; i++ )
		{
			size = tr.framebuffers.luminance[i]->colorTextures[0]->width;

			R_EXT_GLSL_SetUniform1f( tr.glsl.downscaleLuminance, "PixelWidth", 1.0f / (float)size );
			R_EXT_GLSL_SetUniform1f( tr.glsl.downscaleLuminance, "PixelHeight", 1.0f / (float)size );

			R_EXT_BindFramebuffer( tr.framebuffers.luminance[i] );
			GL_Bind( tr.framebuffers.luminance[i-1]->colorTextures[0]->q3image );
			ResizeTarget( size, size );

			DrawFullscreenQuad();
		}

		qglPopAttrib();

		// Tonemapping.
		CalculateLightAdaptation();

		R_EXT_BindFramebuffer( tr.framebuffers.tonemapping );
		R_EXT_GLSL_UseProgram( tr.glsl.tonemapping );
		R_EXT_GLSL_SetUniform1f( tr.glsl.tonemapping, "Exposure", r_postprocess_hdrExposureBias->value/adaptedFrameLuminance );
		GL_Bind( lastFullscreenFBO->colorTextures[0]->q3image );
        DrawFullscreenQuad();
		lastFullscreenFBO = tr.framebuffers.tonemapping;
	}


	//================================
	//	Bloom
	//================================

	if ( r_postprocess_bloom->integer )
	{
		framebuffer_t *lastFBO = tr.framebuffers.brightPass;
		glslProgram_t *shader = NULL;
		float size = 1.0f;

		//Bright pass
		R_EXT_BindFramebuffer( tr.framebuffers.brightPass );
		R_EXT_GLSL_UseProgram( tr.glsl.brightPass );
		R_EXT_GLSL_SetUniform1f( tr.glsl.brightPass, "Threshold", Q_clamp( 0.0f, r_postprocess_brightPassThres->value, 1.0f ) );
		GL_Bind( lastFullscreenFBO->colorTextures[0]->q3image );
		DrawFullscreenQuad();

		R_EXT_GLSL_UseProgram( NULL );
		qglPushAttrib( GL_VIEWPORT_BIT|GL_SCISSOR_BIT );

		for ( i=0, size=1.0f; i<tr.postprocessing.bloomSteps; i++, size *= BLOOM_DOWNSCALE_RATE )
		{// downscale
			R_EXT_BindFramebuffer( tr.framebuffers.bloomDownscale[i] );
			GL_Bind( lastFBO->colorTextures[0]->q3image );
			ResizeTarget( (int)floorf( tr.postprocessing.bloomSize/size ), (int)floorf( tr.postprocessing.bloomSize/size ) );
			DrawFullscreenQuad();
			lastFBO = tr.framebuffers.bloomDownscale[i];
		}

		for ( i=0; i<2; i++ )
		{// blur
			switch ( r_postprocess_bloomBlurStyle->integer )
			{
			default:
			case 1:
				shader = tr.glsl.gaussianBlur3x3[i];
				break;
			case 2:
				shader = tr.glsl.gaussianBlur6x6[i];
				break;
			case 3:
				shader = (i&1) ? tr.glsl.gaussianBlur3x3[i] : tr.glsl.gaussianBlur6x6[i];
				break;
			case 4:
				shader = (i&1) ? tr.glsl.gaussianBlur6x6[i] : tr.glsl.gaussianBlur3x3[i];
				break;
			}

			R_EXT_BindFramebuffer( tr.framebuffers.bloomBlur[i] );

			R_EXT_GLSL_UseProgram( shader );
			if ( !(i & 1) )	R_EXT_GLSL_SetUniform1f( shader, "PixelWidth", 1.0f/floorf(tr.postprocessing.bloomSize/size) );
			else			R_EXT_GLSL_SetUniform1f( shader, "PixelHeight", 1.0f/floorf(tr.postprocessing.bloomSize/size) );

			GL_Bind( lastFBO->colorTextures[0]->q3image );
			if ( !(i & 1) )
				ResizeTarget( (int)floorf( tr.postprocessing.bloomSize/size ), (int)floorf( tr.postprocessing.bloomSize/size ) );
			DrawFullscreenQuad();
			lastFBO = tr.framebuffers.bloomBlur[i];
		}

		R_EXT_GLSL_UseProgram( NULL ); //reset the active shader

		for ( i=tr.postprocessing.bloomSteps-1, size/=BLOOM_DOWNSCALE_RATE; i>=0; i--, size /= BLOOM_DOWNSCALE_RATE )
		{// upscale
			R_EXT_BindFramebuffer( tr.framebuffers.bloomDownscale[i] );
			GL_Bind( lastFBO->colorTextures[0]->q3image );
			ResizeTarget( (int)floorf( tr.postprocessing.bloomSize/size ), (int)floorf( tr.postprocessing.bloomSize/size ) );
			DrawFullscreenQuad();
			lastFBO = tr.framebuffers.bloomDownscale[i];
		}

		//finally upscale into the brightpass fbo (full screen size)
		R_EXT_BindFramebuffer( tr.framebuffers.brightPass );
		GL_Bind( lastFBO->colorTextures[0]->q3image );
		ResizeTarget( (int)w, (int)h );
		DrawFullscreenQuad();
		qglPopAttrib();

		// add the bloom texture onto the scene texture
		R_EXT_BindFramebuffer( tr.framebuffers.bloom );
		R_EXT_GLSL_UseProgram( tr.glsl.bloom );
		GL_SelectTexture( 0 );	GL_Bind( lastFullscreenFBO->colorTextures[0]->q3image );			R_EXT_GLSL_SetUniform1i( tr.glsl.bloom, "SceneTex", 0 );
		GL_SelectTexture( 1 );	GL_Bind( tr.framebuffers.brightPass->colorTextures[0]->q3image );	R_EXT_GLSL_SetUniform1i( tr.glsl.bloom, "BloomTex", 1 );
		R_EXT_GLSL_SetUniform1f( tr.glsl.bloom, "BloomMulti", Q_bump( 0.0f, r_postprocess_bloomMulti->value ) );
		R_EXT_GLSL_SetUniform1f( tr.glsl.bloom, "BloomSat", r_postprocess_bloomSat->value );
		R_EXT_GLSL_SetUniform1f( tr.glsl.bloom, "SceneMulti", Q_bump( 0.0f, r_postprocess_bloomSceneMulti->value ) );
		R_EXT_GLSL_SetUniform1f( tr.glsl.bloom, "SceneSat", r_postprocess_bloomSceneSat->value );
		DrawFullscreenQuad();
		lastFullscreenFBO = tr.framebuffers.bloom;
		GL_SelectTexture( 0 );
	}


	//================================
	//	Final Pass (Colour correction, filters, funky stuff)
	//================================

	if ( r_postprocess_finalPass->integer )
	{
		vector4 userVec1 = { 0.0f };

		R_EXT_BindFramebuffer( tr.framebuffers.final );
		R_EXT_GLSL_UseProgram( tr.glsl.final );
		R_EXT_GLSL_SetUniform1f( tr.glsl.final, "PixelWidth", 1.0f/floorf(glConfig.vidWidth) );
		R_EXT_GLSL_SetUniform1f( tr.glsl.final, "PixelHeight", 1.0f/floorf(glConfig.vidHeight) );
		R_EXT_GLSL_SetUniform4f( tr.glsl.final, "ColourGrading", tr.postprocessing.grading.r, tr.postprocessing.grading.g, tr.postprocessing.grading.b, tr.postprocessing.grading.a );
		if ( sscanf( r_postprocess_userVec1->string, "%f %f %f %f", &userVec1.x, &userVec1.y, &userVec1.z, &userVec1.w ) == 4 )
			R_EXT_GLSL_SetUniform4f( tr.glsl.final, "UserVec1", userVec1.x, userVec1.y, userVec1.z, userVec1.w );
		else
			R_EXT_GLSL_SetUniform4f( tr.glsl.final, "UserVec1", tr.postprocessing.grading.r, tr.postprocessing.grading.g, tr.postprocessing.grading.b, tr.postprocessing.grading.a );
		GL_Bind( lastFullscreenFBO->colorTextures[0]->q3image );
		DrawFullscreenQuad();
		lastFullscreenFBO = tr.framebuffers.final;
	}


	//================================
	//	DISPLAY
	//================================

	R_EXT_GLSL_UseProgram( NULL );
	R_EXT_BlitFramebufferColor( lastFullscreenFBO, NULL, (int)w, (int)h, (int)w, (int)h );
	//	R_EXT_BindFramebuffer( tr.framebuffers.glow );
	//	qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	R_EXT_BindDefaultFramebuffer();
	qglDrawBuffer( GL_BACK );

	#ifdef _DEBUG
		if ( r_postprocess_debugFBOs->integer )
		{
#define ROWS (6.0f)
#define COLUMNS (6.0f)
			int dViews = 0;
			const float weights[36][2] =
			{
				//row 1
				{ (SCREEN_WIDTH/COLUMNS)*0, (SCREEN_HEIGHT/ROWS)*0 },
				{ (SCREEN_WIDTH/COLUMNS)*0,	(SCREEN_HEIGHT/ROWS)*1 },
				{ (SCREEN_WIDTH/COLUMNS)*0,	(SCREEN_HEIGHT/ROWS)*2 },
				{ (SCREEN_WIDTH/COLUMNS)*0,	(SCREEN_HEIGHT/ROWS)*3 },
				{ (SCREEN_WIDTH/COLUMNS)*0,	(SCREEN_HEIGHT/ROWS)*4 },
				{ (SCREEN_WIDTH/COLUMNS)*0,	(SCREEN_HEIGHT/ROWS)*5 },

				//row 2
				{ (SCREEN_WIDTH/COLUMNS)*1, (SCREEN_HEIGHT/ROWS)*0 },
				{ (SCREEN_WIDTH/COLUMNS)*1,	(SCREEN_HEIGHT/ROWS)*1 },
				{ (SCREEN_WIDTH/COLUMNS)*1,	(SCREEN_HEIGHT/ROWS)*2 },
				{ (SCREEN_WIDTH/COLUMNS)*1,	(SCREEN_HEIGHT/ROWS)*3 },
				{ (SCREEN_WIDTH/COLUMNS)*1,	(SCREEN_HEIGHT/ROWS)*4 },
				{ (SCREEN_WIDTH/COLUMNS)*1,	(SCREEN_HEIGHT/ROWS)*5 },

				//row 3
				{ (SCREEN_WIDTH/COLUMNS)*2, (SCREEN_HEIGHT/ROWS)*0 },
				{ (SCREEN_WIDTH/COLUMNS)*2,	(SCREEN_HEIGHT/ROWS)*1 },
				{ (SCREEN_WIDTH/COLUMNS)*2,	(SCREEN_HEIGHT/ROWS)*2 },
				{ (SCREEN_WIDTH/COLUMNS)*2,	(SCREEN_HEIGHT/ROWS)*3 },
				{ (SCREEN_WIDTH/COLUMNS)*2,	(SCREEN_HEIGHT/ROWS)*4 },
				{ (SCREEN_WIDTH/COLUMNS)*2,	(SCREEN_HEIGHT/ROWS)*5 },

				//row 4
				{ (SCREEN_WIDTH/COLUMNS)*3, (SCREEN_HEIGHT/ROWS)*0 },
				{ (SCREEN_WIDTH/COLUMNS)*3,	(SCREEN_HEIGHT/ROWS)*1 },
				{ (SCREEN_WIDTH/COLUMNS)*3,	(SCREEN_HEIGHT/ROWS)*2 },
				{ (SCREEN_WIDTH/COLUMNS)*3,	(SCREEN_HEIGHT/ROWS)*3 },
				{ (SCREEN_WIDTH/COLUMNS)*3,	(SCREEN_HEIGHT/ROWS)*4 },
				{ (SCREEN_WIDTH/COLUMNS)*3,	(SCREEN_HEIGHT/ROWS)*5 },

				//row 5
				{ (SCREEN_WIDTH/COLUMNS)*4, (SCREEN_HEIGHT/ROWS)*0 },
				{ (SCREEN_WIDTH/COLUMNS)*4,	(SCREEN_HEIGHT/ROWS)*1 },
				{ (SCREEN_WIDTH/COLUMNS)*4,	(SCREEN_HEIGHT/ROWS)*2 },
				{ (SCREEN_WIDTH/COLUMNS)*4,	(SCREEN_HEIGHT/ROWS)*3 },
				{ (SCREEN_WIDTH/COLUMNS)*4,	(SCREEN_HEIGHT/ROWS)*4 },
				{ (SCREEN_WIDTH/COLUMNS)*4,	(SCREEN_HEIGHT/ROWS)*5 },

				//row 6
				{ (SCREEN_WIDTH/COLUMNS)*5, (SCREEN_HEIGHT/ROWS)*0 },
				{ (SCREEN_WIDTH/COLUMNS)*5,	(SCREEN_HEIGHT/ROWS)*1 },
				{ (SCREEN_WIDTH/COLUMNS)*5,	(SCREEN_HEIGHT/ROWS)*2 },
				{ (SCREEN_WIDTH/COLUMNS)*5,	(SCREEN_HEIGHT/ROWS)*3 },
				{ (SCREEN_WIDTH/COLUMNS)*5,	(SCREEN_HEIGHT/ROWS)*4 },
				{ (SCREEN_WIDTH/COLUMNS)*5,	(SCREEN_HEIGHT/ROWS)*5 },
			};
			float	w = (SCREEN_WIDTH/COLUMNS);
			float	h = (SCREEN_HEIGHT/ROWS);
			framebuffer_t **fbo = (framebuffer_t**)&tr.framebuffers;
			int i=0;

			while ( i<sizeof( tr.framebuffers ) / sizeof( framebuffer_t* ) )
			{
				if ( *fbo )
				{
					GL_Bind( (*fbo)->colorTextures[0]->q3image );
					DrawQuad( weights[dViews][0], weights[dViews][1], w, h );
					dViews++;
				}
				fbo++;
				i++;
			}
		}
		#undef DEBUGVIEW
	#endif

	return;
}

void R_PostProcessReload_f( void ) {
	R_EXT_Cleanup();
	R_EXT_Init();
}
