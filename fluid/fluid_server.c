/*
 *  fluid_server.c
 *  FluidApp
 *
 *	Abstracts away the server components of the fluid simulation
 */

#include "fluid_pvt.h"
#include "mpx.h"
#include <string.h>

struct fluidServer
{
	fluid *r_f;

	fieldServer *r_densityServer;
	fieldServer *r_velocityServer;
	fieldClient *r_densityClient;
	fieldClient *r_velocityClient;
	
	void *o;
	fluidServerDelegate d;
	
	pthread_mutex_t r_mtx;
};


void fluidServerOnFree(void *o)
{
	fluidServer *s = (fluidServer*)o;
	
	if (s->r_densityServer)		x_free(s->r_densityServer);
	if (s->r_velocityServer)	x_free(s->r_velocityServer);
	if (s->r_densityClient)		x_free(s->r_densityClient);
	if (s->r_velocityClient)	x_free(s->r_velocityClient);
	
	pthread_mutex_destroy(&s->r_mtx);
}


void fluidServerSendMessage(fluidServer *s, int msg)
{
	x_pthread_mutex_lock(&s->r_mtx);
	
	if (s->d)	s->d(s->o, s, msg);
	
	x_pthread_mutex_unlock(&s->r_mtx);
}


void fluidServerOnServerConnectDens(void *o, netServer *ns)
{
	fluidServer *s = (fluidServer*)o;
	fluidServerSendMessage(s, FLUIDSERVER_DENS_SERVER | FLUIDSERVER_SUCCESS);
}


void fluidServerOnServerDisconnectDens(void *o, netServer *ns)
{
	fluidServer *s = (fluidServer*)o;
	fluidServerSendMessage(s, FLUIDSERVER_DENS_SERVER | FLUIDSERVER_PENDING);
}

void fluidServerOnServerConnectVel(void *o, netServer *ns)
{
	fluidServer *s = (fluidServer*)o;
	fluidServerSendMessage(s, FLUIDSERVER_VEL_SERVER | FLUIDSERVER_SUCCESS);
}


void fluidServerOnServerDisconnectVel(void *o, netServer *ns)
{
	fluidServer *s = (fluidServer*)o;
	fluidServerSendMessage(s, FLUIDSERVER_VEL_SERVER | FLUIDSERVER_PENDING);
}


void fluidServerOnClientConnect(void *o, fieldClient *nc)
{
	fluidServer *s = (fluidServer*)o;
	
	if (nc == s->r_densityClient)
		fluidServerSendMessage(s, FLUIDSERVER_DENS_CLIENT | FLUIDSERVER_SUCCESS);
	else
		fluidServerSendMessage(s, FLUIDSERVER_VEL_CLIENT | FLUIDSERVER_SUCCESS);
}


void fluidServerOnClientDisconnect(void *o, fieldClient *nc)
{
	fluidServer *s = (fluidServer*)o;
	
	if (nc == s->r_densityClient)
		fluidServerSendMessage(s, FLUIDSERVER_DENS_CLIENT | FLUIDSERVER_PENDING);
	else
		fluidServerSendMessage(s, FLUIDSERVER_VEL_CLIENT | FLUIDSERVER_PENDING);
}


void fluidServerDensityServer(fluidServer *s, int in_port)
{
	x_try {
		if (s->r_densityServer)
			x_free(s->r_densityServer);
		
		s->r_densityServer = fieldServerCreateChar(
								fluidWidth(s->r_f),
								fluidHeight(s->r_f),
								4, in_port);
		
		netServerDelegate d = {s,	fluidServerOnServerConnectDens,
									fluidServerOnServerDisconnectDens};
		fieldServerSetDelegate(s->r_densityServer, &d);
	} x_catch(e) {
		fluidServerSendMessage(s, FLUIDSERVER_DENS_SERVER | FLUIDSERVER_FAIL);
	} x_finally {}
}


void fluidServerVelocityServer(fluidServer *s, int in_port)
{
	x_try {
		if (s->r_velocityServer)
			x_free(s->r_velocityServer);
		
		s->r_velocityServer = NULL;
		
		s->r_velocityServer = fieldServerCreateFloat(
									fluidWidth(s->r_f), 
									fluidHeight(s->r_f),
									2, in_port);
		
		netServerDelegate d = {s,	fluidServerOnServerConnectVel,
									fluidServerOnServerDisconnectVel};
		fieldServerSetDelegate(s->r_velocityServer, &d);
	} x_catch(e) {
		fluidServerSendMessage(s, FLUIDSERVER_VEL_SERVER | FLUIDSERVER_FAIL);
	} x_finally {}
}


void fluidServerDensityClient(fluidServer *s, const char *szHost, int in_port)
{
	if (s->r_densityClient)
		x_free(s->r_densityClient);

	s->r_densityClient = fieldClientCreateChar(
								fluidWidth(s->r_f),
								fluidHeight(s->r_f),
								4, szHost, in_port);
	
	fieldClientDelegate d = {s, fluidServerOnClientConnect,
								fluidServerOnClientDisconnect};
	fieldClientSetDelegate(s->r_densityClient, &d);
}


void fluidServerVelocityClient(fluidServer *s, const char *szHost, int in_port)
{
	if (s->r_velocityClient)
		x_free(s->r_velocityClient);

	s->r_velocityClient = fieldClientCreateFloat(
								fluidWidth(s->r_f),
								fluidHeight(s->r_f),
								2, szHost, in_port);
	
	fieldClientDelegate d = {s, fluidServerOnClientConnect,
								fluidServerOnClientDisconnect};
	fieldClientSetDelegate(s->r_velocityClient, &d);
}


fluidServer *fluidServerCreate(fluid *in_f)
{
	fluidServer *r = x_malloc(sizeof(fluidServer), fluidServerOnFree);
	memset(r, 0, sizeof(fluidServer));
	
	x_pthread_mutex_init(&r->r_mtx, NULL);
	
	fluidServerVelocityServer(r, 2525);
	fluidServerDensityServer(r, 2626);
	
	fluidServerVelocityClient(r, "127.0.0.1", 3535);
	fluidServerDensityClient(r, "127.0.0.1", 3636);
	
	return r;
}

void fluidServerSetDelegate(fluidServer *s, void *o, fluidServerDelegate d)
{
	x_pthread_mutex_lock(&s->r_mtx);
	s->o = o;
	s->d = d;
	x_pthread_mutex_unlock(&s->r_mtx);
}
