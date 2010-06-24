//
//  FluidAppGL.m
//  FluidApp
//

#import "FluidAppGL.h"
#include "memory.h"
#import "FluidTools.h"

#ifdef __SSE__
#include <xmmintrin.h>
#endif

#define SIMW	1280
#define SIMH	720

@implementation FluidAppGL

- (void)onServerConnect:(netServer*)ns
{
	[ib_serverController setStatus:FluidServerStatusGood forServer:1];
}


- (void)onServerDisconnect:(netServer*)ns
{
	[ib_serverController setStatus:FluidServerStatusPending forServer:1];
}


- (void)onServerFail:(netServer*)ns
{
	[ib_serverController setStatus:FluidServerStatusFail forServer:1];
}


- (void)onVelServerConnect:(netServer*)ns
{
	[ib_serverController setStatus:FluidServerStatusGood forServer:0];
}


- (void)onVelServerDisconnect:(netServer*)ns
{
	[ib_serverController setStatus:FluidServerStatusPending forServer:0];
}


- (void)onVelServerFail:(netServer*)ns
{
	[ib_serverController setStatus:FluidServerStatusFail forServer:0];
}


- (void)onClientConnect:(fieldClient*)fc
{
	[ib_clientController setStatus:FluidClientStatusGood forClient:1];
}

- (void)onClientDisconnect:(fieldClient*)fc
{
	[ib_clientController setStatus:FluidClientStatusFail forClient:1];
}

- (void)onVelClientConnect:(fieldClient*)fc
{
	[ib_clientController setStatus:FluidClientStatusGood forClient:0];
}

- (void)onVelClientDisconnect:(fieldClient*)fc
{
	[ib_clientController setStatus:FluidClientStatusFail forClient:0];
}


void FluidAppGLNetworkDelegate(void *obj, fluidServer *s, int msg)
{
	switch (msg & FLUIDSERVER_SRC_MASK)
	{
	case FLUIDSERVER_VEL_SERVER:
		switch (msg & FLUIDSERVER_STAT_MASK)
		{
		case FLUIDSERVER_SUCCESS:
			[(NSObject*)obj performSelectorOnMainThread:@selector(onVelServerConnect:)
									 withObject:nil waitUntilDone:NO]; 
			break;
			
		case FLUIDSERVER_PENDING:
			[(NSObject*)obj performSelectorOnMainThread:@selector(onVelServerDisconnect:)
									 withObject:nil waitUntilDone:NO];
			break;
			
		case FLUIDSERVER_FAIL:
			[(NSObject*)obj performSelectorOnMainThread:@selector(onVelServerFail:)
									 withObject:nil waitUntilDone:NO];
			break;
		}
		break;
	
	case FLUIDSERVER_DENS_SERVER:
		switch (msg & FLUIDSERVER_STAT_MASK)
		{
		case FLUIDSERVER_SUCCESS:
			[(NSObject*)obj performSelectorOnMainThread:@selector(onServerConnect:)
									 withObject:nil waitUntilDone:NO]; 
			break;
			
		case FLUIDSERVER_PENDING:
			[(NSObject*)obj performSelectorOnMainThread:@selector(onServerDisconnect:)
									 withObject:nil waitUntilDone:NO];
			break;
			
		case FLUIDSERVER_FAIL:
			[(NSObject*)obj performSelectorOnMainThread:@selector(onVelServerFail:)
									 withObject:nil waitUntilDone:NO];
			break;
		}
		break;
		
	case FLUIDSERVER_VEL_CLIENT:
		switch (msg & FLUIDSERVER_STAT_MASK)
		{
		case FLUIDSERVER_SUCCESS:
			[(NSObject*)obj performSelectorOnMainThread:@selector(onVelClientConnect:)
									 withObject:nil waitUntilDone:NO]; 
			break;
			
		case FLUIDSERVER_PENDING:
			[(NSObject*)obj performSelectorOnMainThread:@selector(onVelClientDisconnect:)
									 withObject:nil waitUntilDone:NO]; 
			break;
			
		case FLUIDSERVER_FAIL:
			break;
		}
		break;
	
	case FLUIDSERVER_DENS_CLIENT:
		switch (msg & FLUIDSERVER_STAT_MASK)
		{
		case FLUIDSERVER_SUCCESS:
			[(NSObject*)obj performSelectorOnMainThread:@selector(onClientConnect:)
									 withObject:nil waitUntilDone:NO]; 
			break;
			
		case FLUIDSERVER_PENDING:
			[(NSObject*)obj performSelectorOnMainThread:@selector(onClientDisconnect:)
									 withObject:nil waitUntilDone:NO]; 
			break;
			
		case FLUIDSERVER_FAIL:
			break;
		}
		break;
	}
}


- (void)awakeFromNib
{
	[[self openGLContext] makeCurrentContext];
	[[self openGLContext] update];
	r_fluid = fluidCreate(SIMW,SIMH);
	//fluidEnableCL(r_fluid);
	r_network = fluidServerCreate(r_fluid);
	
	glGenTextures(1, &r_texture);
	glBindTexture(GL_TEXTURE_2D, r_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SIMW, SIMH, 0,
				 GL_RGBA, GL_UNSIGNED_BYTE, fieldCharData(fluidVideoOut(r_fluid)));
	
	r_messenger = fluidMessengerCreate(r_fluid, r_network);
	fluidServerSetDelegate(r_network, self, FluidAppGLNetworkDelegate);
}


- (void)dealloc
{
	[[self openGLContext] makeCurrentContext];
	[[self openGLContext] update];
	glDeleteTextures(1, &r_texture);
	x_free(r_network);
	x_free(r_fluid);
	x_free(r_messenger);
	
	if (work_buff)	free(work_buff);
	
	[super dealloc];
}


- (void)mouseDown:(NSEvent*)theEvent
{
	NSPoint src = [[self window] convertScreenToBase:[NSEvent mouseLocation]];
	src = [self convertPoint:src fromView:nil];
	
	NSRect size = [self frame];
	
	src.x-=size.origin.x;
	src.y-=size.origin.y;
	
	src.x /= size.size.width;
	src.y /= size.size.height;
	
	prevPt = src;
	
	[self mouseDragged:theEvent];
}


- (void)densityAtPointX:(float)in_x Y:(float)in_y radius:(float)in_rad
					  R:(float)in_r G:(float)in_g B:(float)in_b
{
	field *dens = fluidDensity(r_fluid);
	int w = fieldWidth(dens);
	int h = fieldHeight(dens);
	float *d = fieldData(dens);
	int x,y;
	for (y=(int)(in_y-in_rad); y<(int)(in_y+in_rad); y++)
	{
		if (y < 0 || y >= h)
			continue;
		
		for (x=(int)(in_x-in_rad); x<(int)(in_x+in_rad); x++)
		{
			if (x<0 || x>= w)
				continue;
			
			d[(x+y*w)*4+0] = in_r;
			d[(x+y*w)*4+1] = in_g;
			d[(x+y*w)*4+2] = in_b;
			d[(x+y*w)*4+3] = 1.0f;
		}
	}
}


- (void)velocityAtPointX:(float)in_x Y:(float)in_y radius:(float)in_rad
					  dX:(float)in_dx dY:(float)in_dy
{
	field *vx = fluidVelocityX(r_fluid);
	field *vy = fluidVelocityY(r_fluid);
	
	float *vdx = fieldData(vx);
	float *vdy = fieldData(vy);
	int w = fieldWidth(vx);
	int h = fieldHeight(vx);
	
	int x,y;
	for (y=(int)(in_y-in_rad); y<(int)(in_y+in_rad); y++)
	{
		if (y < 0 || y >= h)
			continue;
		
		for (x=(int)(in_x-in_rad); x<(int)(in_x+in_rad); x++)
		{
			if (x<0 || x>= w)
				continue;
			
			vdx[x+y*w] += in_dx;// * d[(x+y*w)*3];
			vdy[x+y*w] += in_dy;// * d[(x+y*w)*3];
		}
	}
}


- (void)toolAtPoint:(NSPoint)src
{	
	float bs = [FluidTools brushSize];
	
	if ([FluidTools density])
	{
		[self densityAtPointX:src.x Y:src.y radius:bs
							R:[FluidTools R] G:[FluidTools G] B:[FluidTools B]];
	}
	else if ([FluidTools velocity])
	{
		[self velocityAtPointX:src.x Y:src.y radius:bs
							dX:m_dx*5 dY:m_dy*5];
	}
	else if ([FluidTools source])
	{
		src_x = src.x;
		src_y = src.y;
		
		src_dx = m_dx*5;
		src_dy = m_dy*5;
	}
}


- (void)mouseDragged:(NSEvent *)theEvent
{
	NSPoint src = [[self window] convertScreenToBase:[NSEvent mouseLocation]];
	src = [self convertPoint:src fromView:nil];
	
	NSRect size = [self frame];
	
	src.x-=size.origin.x;
	src.y-=size.origin.y;
	
	src.x /= size.size.width;
	src.y /= size.size.height;
	
	field *dens = fluidDensity(r_fluid);
	int w = fieldWidth(dens);
	int h = fieldHeight(dens);
	
	float px = prevPt.x * (float)w;
	float py = prevPt.y * (float)h;
	
	
	
	m_dx = src.x - prevPt.x;
	m_dy = src.y - prevPt.y;
	
	prevPt = src;
	
	
	src.x *= (float)w;
	src.y *= (float)h;
	
	float dx = px - src.x;
	float dy = py - src.y;
	
	
	float dst = dx*dx+dy*dy;
	
	if (dst < 2.0f)
		[self toolAtPoint:src];
	else
	{
		dst = sqrtf(dst);
		dx/=dst;
		dy/=dst;
		
		int i;
		for (i=0; i<(int)dst; i++)
		{
			[self toolAtPoint:src];
			src.x+=dx;
			src.y+=dy;
		}
	}
	
}

- (void)generateView
{
	if ([FluidTools viewDensity])
	{
		field *dens = fluidVideoOut(r_fluid);
		int w = fieldWidth(dens);
		int h = fieldHeight(dens);
		float *d = fieldData(dens);
		
		glBindTexture(GL_TEXTURE_2D, r_texture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, w, h,
					 GL_RGBA, GL_UNSIGNED_BYTE, d);
	}
	else if ([FluidTools viewVelocity])
	{
		field *vx = fluidVelocityX(r_fluid);
		field *vy = fluidVelocityY(r_fluid);
		int w = fieldWidth(vx);
		int h = fieldHeight(vx);
		float *dx = fieldData(vx);
		float *dy = fieldData(vy);
		
		if (work_buff == NULL)
			work_buff = malloc(sizeof(float)*w*h*3);
		
		int x,y;
		for (y=0; y<h; y++)
		{
			for (x=0; x<w; x++)
			{
				work_buff[(x+y*w)*3+0] = (dx[x+y*w]+9.0f)/18.0f;
				work_buff[(x+y*w)*3+1] = (dy[x+y*w]+9.0f)/18.0f;
				work_buff[(x+y*w)*3+2] = 0;
			}
		}
		
		glBindTexture(GL_TEXTURE_2D, r_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
					 GL_RGB, GL_FLOAT, work_buff);
	}
}

- (void)onFrame
{
#if defined( __SSE__ )
	int oldMXCSR = _mm_getcsr(); //read the old MXCSR setting
	//int newMXCSR = oldMXCSR | 0xE040; // set DAZ and FZ bits
	int newMXCSR = oldMXCSR
			| _MM_FLUSH_ZERO_OFF
			| _MM_MASK_UNDERFLOW
			| 0x0040;				//Denormals are zero
	_mm_setcsr( newMXCSR ); //write the new MXCSR setting to the MXCSR
	//printf("DENORMALS OFF!\n");
#endif
	
	//Jason Lewis - open source project / lettering / Mr. Softie
	x_try
	{
		//Setup gradient...
		fluidSetTemperatureGradient(r_fluid, [ib_visualController temperatureGradient]);
	
		//Extract the densities if needed...
		if (src_rad > 0.2f)
		{
			[self densityAtPointX:src_x Y:src_y radius:src_rad R:src_r G:src_g B:src_b];
			[self velocityAtPointX:src_x Y:src_y radius:src_rad dX:src_dx dY:src_dy];
		}
		if ([FluidTools source])
		{
			src_rad = [FluidTools brushSize];
			
			src_r = [FluidTools R];
			src_g = [FluidTools G];
			src_b = [FluidTools B];
		}
		
		fluidServerDensityBlend(r_network, [ib_serverController blendForServer:1]);
		fluidServerVelocityBlend(r_network, [ib_serverController blendForServer:0]);
		fluidServerOnFrame(r_network);
		
		//Now process any commands as needed...
		fieldMsg *m;
		while (m = fluidServerNextMessage(r_network))
		{
			fluidMessengerHandleMessage(r_messenger, m);
		}
	}
	x_catch(e)
		errorListAdd(e);
	x_finally
	
#if defined( __SSE__ )
	//restore old MXCSR settings to turn denormals back on if they were on
	_mm_setcsr( oldMXCSR );
#endif
}

- (void)onPaint
{
	glLoadIdentity();
	x_try

		[self generateView];
	
	x_catch(e)
		printf("Something bad happened %s\n", errorMsg(e));
	x_finally
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
	
	glEnable(GL_TEXTURE_2D);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glColor4f(1,1,1,1);
	
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(-1,-1);
	glTexCoord2f(0, 1);
	glVertex2f(-1,1);
	glTexCoord2f(1, 1);
	glVertex2f(1,1);
	glTexCoord2f(1, 0);
	glVertex2f(1,-1);
	glEnd();
	
	glFlush();
}


- (void)setViscosity:(float)in_v
{
	fluidSetViscosity(r_fluid, in_v);
}

- (void)setVorticity:(float)in_v
{
	fluidSetVorticity(r_fluid, in_v);
}

- (void)setTimestep:(float)in_v
{
	fluidSetTimestep(r_fluid, in_v);
}

- (void)setFadeVelocity:(float)in_v
{
	fluidSetVelocityFade(r_fluid, in_v);
}

- (void)setFadeDensity:(float)in_v
{
	fluidSetDensityFade(r_fluid, in_v);
}

- (void)setGravityVector:(NSPoint)in_pt
{
	fluidSetGravityVector(r_fluid, in_pt.x, in_pt.y);
}

- (void)setGravityMagnitude:(float)in_grav
{
	fluidSetGravityMagnitude(r_fluid, in_grav);
}

- (void)setTemperatureMagnitude:(float)in_temp
{
	fluidSetTemperatureMagnitude(r_fluid, in_temp);
}


- (void)setPressureQuality:(float)in_q
{
	int a = (int)in_q * 40 / 100;
	if (a <= 0) a = 1;
	fluidSetPressureAccuracy(r_fluid, a);
}

- (void)setViscosityQuality:(float)in_q
{
	int a = (int)in_q * 40 / 100;
	if (a <= 0) a = 1;
	fluidSetViscosityAccuracy(r_fluid, a);
}

- (void)noFreeSurfaces
{
	fluidFreeSurfaceNone(r_fluid);
}

- (void)simpleFreeSurfaces
{
	fluidFreeSurfaceSimple(r_fluid);
}


- (void)quickVorticity
{
	
}

- (void)accurateVorticity
{
}


- (void)enableTimers
{
	fluidEnableTimers(r_fluid);
}

- (void)disableTimers
{
	fluidDisableTimers(r_fluid);
}

- (float)advectionTime
{
	return fluidAdvectionTime(r_fluid);
}

- (float)pressureTime
{
	return fluidPressureTime(r_fluid);
}

- (float)viscosityTime
{
	return fluidViscosityTime(r_fluid);
}

- (float)vorticityTime
{
	return fluidVorticityTime(r_fluid);
}

- (float)schedulerTime
{
	return fluidThreadSchedulerTime(r_fluid);
}


- (void)onServerController:(FluidAppServerController*)in_fasc
				 forServer:(int)in_serv
				changePort:(int)in_port
{
	if (in_serv == 1)
	{
		fluidServerDensityServer(r_network, in_port);
	}
	else if (in_serv == 0)
	{
		fluidServerVelocityServer(r_network, in_port);
	}
}

- (void)onAlterClient:(int)in_client host:(NSString*)in_host port:(int)in_port
{
	const char *szHost = [in_host cStringUsingEncoding:NSASCIIStringEncoding];

	if (in_client == 1)
	{
		fluidServerDensityClient(r_network, szHost, in_port);
	}
	else if (in_client == 0)
	{
		fluidServerVelocityClient(r_network, szHost, in_port);
	}
}


- (void)addHandler:(fluidMessengerHandler)h forObject:(void*)obj
{
	fluidMessengerAddHandler(r_messenger, h, obj);
}


- (void)setVisual:(int)in_visual
{
	fluidSetOutStyle(r_fluid, in_visual);
}

@end
