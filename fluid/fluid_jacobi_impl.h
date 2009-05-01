/*
 *  fluid_jacobi_impl.h
 *  FluidApp
 */

//
//	Useful ifDefs:
//		FSJacobi_Error_Report
//		-	Adds an extra parameter to the code, allowing it to return
//			the error information in an array that's passed in.
//

#include "fluid_jacobi.h"
#include "fluid_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef tFS_Name
	#define t_FS_JP_Name FSConcat(fluidJacobiPressure, tFS_Name)
	#define t_FS_JD_Name FSConcat(fluidJacobiDiffusion, tFS_Name)
	#define t_FS_JP_ALTIVEC_Name FSConcat(fluidJacobiPressure_Altivec, tFS_Name)
	#define t_FS_JD_ALTIVEC_Name FSConcat(fluidJacobiDiffusion_Altivec, tFS_Name)
#else
	#define t_FS_JP_Name fluidJacobiPressure
	#define t_FS_JD_Name fluidJacobiDiffusion
	#define t_FS_JP_ALTIVEC_Name fluidJacobiPressure_AltiVec
	#define t_FS_JD_ALTIVEC_Name fluidJacobiDiffusion_AltiVec
#endif


#define INT32_MASK	0xFFFFFFFF

#ifdef __APPLE_ALTIVEC__
error *t_FS_JP_ALTIVEC_Name(int in_nIterations, mpTaskSet *in_seq,
						  int32_t *atomicY,
						  fluidStreamDesc velocity,
						  fluidStreamDesc collision,
						  fluidStreamDesc pressure[]
#ifdef FSJacobi_Error_Report
						,float out_error[]
#endif
						)
{
	//These vectors will permute the floats so that we can process 4
	//in parallel.  (Note the theorhetical speed boost)
	vector unsigned char perm1 = {	0x0C, 0x0D, 0x0E, 0x0F,
									0x10, 0x11, 0x12, 0x13,
									0x14, 0x15, 0x16, 0x17,
									0x18, 0x19, 0x1A, 0x1B};
	vector unsigned char perm2 = {	0x04, 0x05, 0x06, 0x07,
									0x08, 0x09, 0x0A, 0x0B,
									0x0C, 0x0D, 0x0E, 0x0F,
									0x10, 0x11, 0x12, 0x13};
	
	//Vectors need different permutations... (select the Y values)
	vector unsigned char selYs = {	0x04, 0x05, 0x06, 0x07,
									0x0C, 0x0D, 0x0E, 0x0F,
									0x14, 0x15, 0x16, 0x17,
									0x1C, 0x1D, 0x1E, 0x1F};
	vector unsigned char selXs = {	0x00, 0x01, 0x02, 0x03,
									0x08, 0x09, 0x0A, 0x0B,
									0x10, 0x11, 0x12, 0x13,
									0x18, 0x19, 0x1A, 0x1B};
	
	vector float quarter = {0.25f, 0.25f, 0.25f, 0.25f};
	vector unsigned int iVecZero = {0,0,0,0};
	vector float zero = {0,0,0,0};

	FSStreamDesc *p1 = pressure;
	FSStreamDesc *p2 = pressure+1;

#ifdef FSJacobi_Error_Report
	memset(out_error, 0, sizeof(float)*in_nIterations);
#endif
	
	int itrNo,x,y;
	
	//Precompute the velocity permutations as they don't change...
	y = OSAtomicIncrement32Barrier(atomicY)-1;
	for (;y<velocity.height; )
	{		
		int vStrideY = y*velocity.strideY;
		int pStrideY = sizeof(float)*velocity.width*y;
		int colStrideY = y*collision.strideY;
			
		for (x=0; x<velocity.width; x+=4)
		{
			int colStrideX = x*collision.strideX;
			
			//Convert the 'x' and 'y' to new format.  This will likely
			//accelerate the whole process...
			vector float *vVelMiddle1 = (vector float*)
											FSFloatPtrOffset(velocity.data.f,
												  x*8 + vStrideY);
			vector float *vVelMiddle2 = (vector float*)
											FSFloatPtrOffset(velocity.data.f,
												x*8+4 + vStrideY);
			
			//Write out solely the 'x' and 'y' values
			vector float *dstX = (vector float*)
											FSFloatPtrOffset(pressure[2].data.f,
												x*4 + pStrideY);
			vector float *dstY = (vector float*)
											FSFloatPtrOffset(pressure[3].data.f,
												x*4 + pStrideY);
			vector unsigned int *dstC = (vector unsigned int*)
											FSFloatPtrOffset(pressure[4].data.f,
												x*4 + pStrideY);
			
			*dstX = vec_perm(*vVelMiddle1, *vVelMiddle2, selXs);
			*dstY = vec_perm(*vVelMiddle1, *vVelMiddle2, selYs);
			
			unsigned char *ptrCollision = collision.data.c + colStrideX
															+ colStrideY;
			
			*dstC = (vector unsigned int)
					{	ptrCollision[0*collision.strideX] == 0?0:INT32_MASK,
						ptrCollision[1*collision.strideX] == 0?0:INT32_MASK,
						ptrCollision[2*collision.strideX] == 0?0:INT32_MASK,
						ptrCollision[3*collision.strideX] == 0?0:INT32_MASK };
		}
		
		y = OSAtomicIncrement32Barrier(atomicY)-1;
	}
	
	[in_seq waitForTask];
	
	//Good, now we iterate...
	for (itrNo =0; itrNo < in_nIterations; itrNo++)
	{
		y = OSAtomicIncrement32Barrier(atomicY)-1;
#ifdef FSJacobi_Error_Report
		vector float error = {0,0,0,0};
#endif
		
		for (; y<velocity.height;)
		{
			int yp1 = (y+1)%velocity.height;
			int ym1 = (y+velocity.height-1)%velocity.height;
			
			int colStrideY = y*collision.strideY;
			int colStrideYM1 = ym1*collision.strideY;
			int colStrideYP1 = yp1*collision.strideY;
			
			int p1StrideY = y*p1->strideY;
			int p2StrideY = y*p2->strideY;
			
			int p2StrideYP1 = yp1*p2->strideY;
			int p2StrideYM1 = ym1*p2->strideY;
			
			vec_dst(FSFloatPtrOffset(p2->data.f,p2StrideY+255), 0x02000010, 0);
			vec_dst(FSFloatPtrOffset(pressure[2].data.f,p2StrideY+255), 0x02000010, 1);
			vec_dst(FSFloatPtrOffset(pressure[3].data.f,p2StrideY+255), 0x02000010, 2);
			
			int nxt=256;
			
			//X is incremented by 4 as the width of a vector is 4 floats.
			//	(same for SSE and AltiVec)
			for(x=0; x<velocity.width; x+=4)
			{			
				int p1StrideX = x*p1->strideX;
				int p2StrideX = x*p2->strideX;
				int colStrideX = x*collision.strideX;
			
				int p2StrideXM1;
				int p2StrideXP1;
				int colStrideXM1;	//The collisions will slow this process down...
				int colStrideXP1;	//	... if fetch time is too long!
				
				if (x == 0)
				{
					p2StrideXM1 = (velocity.width-4)*p2->strideX;
					p2StrideXP1 = (x+4)*p2->strideX;
					colStrideXM1 = (velocity.width-1)*collision.strideX;
					colStrideXP1 = (x+4)*collision.strideX;
				}
				else if (x == velocity.width-4)
				{
					p2StrideXM1 = (x-4)*p2->strideX;
					p2StrideXP1 = (0)*p2->strideX;
					colStrideXM1 = (x-1)*collision.strideX;
					colStrideXP1 = (0)*collision.strideX;
				}
				else
				{
					p2StrideXM1 = (x-4)*p2->strideX;
					p2StrideXP1 = (x+4)*p2->strideX;
					colStrideXM1 = (x-1)*collision.strideX;
					colStrideXP1 = (x+4)*collision.strideX;
					
					//Give hints to the cache about what we'll read next!
					if (x > nxt)
					{
						nxt = x+256;
						vec_dst(FSFloatPtrOffset(p2->data.f,p2StrideY+256+p2StrideX), 0x02000010, 0);
						vec_dst(FSFloatPtrOffset(pressure[2].data.f,p2StrideY+256+p2StrideX), 0x02000010, 1);
						vec_dst(FSFloatPtrOffset(pressure[3].data.f,p2StrideY+256+p2StrideX), 0x02000010, 2);
					}
				}
				
				
				//Get the vectors (src pressure)
				vector float *vSrcUp = (vector float*)
											FSFloatPtrOffset(p2->data.f,
												  p2StrideX + p2StrideYM1);
				vector float *vSrcDown = (vector float*)
											FSFloatPtrOffset(p2->data.f,
												  p2StrideX + p2StrideYP1);
				vector float *vSrcMiddle = (vector float*)
											FSFloatPtrOffset(p2->data.f,
												  p2StrideX + p2StrideY);
				vector float *vSrcLeft = (vector float*)
											FSFloatPtrOffset(p2->data.f,
												  p2StrideXM1 + p2StrideY);
				vector float *vSrcRight = (vector float*)
											FSFloatPtrOffset(p2->data.f,
												  p2StrideXP1 + p2StrideY);
				
				//Get the vectors (src velocity)
				vector float *vVelUp = (vector float*)
											FSFloatPtrOffset(pressure[3].data.f,
												  p2StrideX + p2StrideYM1);
				vector float *vVelDown = (vector float*)
											FSFloatPtrOffset(pressure[3].data.f,
												  p2StrideX + p2StrideYP1);
				vector float *vVelMiddle = (vector float*)
											FSFloatPtrOffset(pressure[2].data.f,
												  p2StrideX + p2StrideY);
				vector float *vVelLeft = (vector float*)
											FSFloatPtrOffset(pressure[2].data.f,
												  p2StrideXM1 + p2StrideY);
				vector float *vVelRight = (vector float*)
											FSFloatPtrOffset(pressure[2].data.f,
												  p2StrideXP1 + p2StrideY);
				
				//Now patch up the velocity values
				vector float vVelCmpLeft = vec_perm(*vVelLeft, *vVelMiddle, perm1);
				vector float vVelCmpRight = vec_perm(*vVelMiddle, *vVelRight, perm2);
				
				//Get the vectors (dst pressure)
				vector float *vDst  = (vector float*)
										FSFloatPtrOffset(p1->data.f,
											   p1StrideX + p1StrideY);
					
				//Faster to do 4 compares...
				//	- Actually, faster to pre-compute.
				vector unsigned int *collisionMaskMiddle = (vector unsigned int*)
											FSFloatPtrOffset(pressure[4].data.f,
												  p2StrideX + p2StrideY);
				
				//Data-level parallelism doesn't really like if-statements.
				//	(this is where velocity engine will slow down)
				vector float vCmpSrcLeft = vec_perm(*vSrcLeft, *vSrcMiddle, perm1);
				vector float vCmpSrcRight = vec_perm(*vSrcMiddle, *vSrcRight, perm2);
				vector float newVal = vec_madd(
										vec_add(
					vec_add(vec_add(*vSrcUp, *vSrcDown),vec_add(vCmpSrcLeft, vCmpSrcRight)),
					vec_add(vec_sub(vVelCmpLeft, vVelCmpRight),vec_sub(*vVelUp,*vVelDown))
					)
					,quarter, zero);
				
			#ifdef FSJacobi_Error_Report
					error = vec_add(error, vec_abs(vec_sub(newVal, *vDst)));
			#endif
				*vDst = newVal;
				
				
				if (!vec_all_eq(*collisionMaskMiddle, iVecZero))
				{
					unsigned char *ptrCollision = collision.data.c + colStrideX
															+ colStrideY;
				
					unsigned char *ptrCollisionUp = collision.data.c + colStrideX
																+ colStrideYM1;
					unsigned char *ptrCollisionDown = collision.data.c + colStrideX
																+ colStrideYP1;
					unsigned char *ptrCollisionLeft = collision.data.c + colStrideXM1
																+ colStrideY;
					unsigned char *ptrCollisionRight = collision.data.c + colStrideXP1
																+ colStrideY;
					vector unsigned int collisionMaskUp =
						{	ptrCollisionUp[0*collision.strideX] != 0?0:INT32_MASK,
							ptrCollisionUp[1*collision.strideX] != 0?0:INT32_MASK,
							ptrCollisionUp[2*collision.strideX] != 0?0:INT32_MASK,
							ptrCollisionUp[3*collision.strideX] != 0?0:INT32_MASK };
					
					vector unsigned int collisionMaskDown =
						{	ptrCollisionDown[0*collision.strideX] != 0?0:INT32_MASK,
							ptrCollisionDown[1*collision.strideX] != 0?0:INT32_MASK,
							ptrCollisionDown[2*collision.strideX] != 0?0:INT32_MASK,
							ptrCollisionDown[3*collision.strideX] != 0?0:INT32_MASK };
					
					vector unsigned int collisionMaskLeft =
						{	ptrCollisionLeft[0*collision.strideX] != 0?0:INT32_MASK,
							ptrCollision[0*collision.strideX] != 0?0:INT32_MASK,
							ptrCollision[1*collision.strideX] != 0?0:INT32_MASK,
							ptrCollision[2*collision.strideX] != 0?0:INT32_MASK };
					
					vector unsigned int collisionMaskRight =
						{	ptrCollision[1*collision.strideX] != 0?0:INT32_MASK,
							ptrCollision[2*collision.strideX] != 0?0:INT32_MASK,
							ptrCollision[3*collision.strideX] != 0?0:INT32_MASK,
							ptrCollisionRight[0*collision.strideX] != 0?0:INT32_MASK };
					
					
					//The collision code - now - is quite special in that it always
					//runs... (go! go! brute force!)
					collisionMaskUp = vec_and(collisionMaskUp, *collisionMaskMiddle);
					collisionMaskDown = vec_and(collisionMaskDown, *collisionMaskMiddle);
					collisionMaskLeft = vec_and(collisionMaskLeft, *collisionMaskMiddle);
					collisionMaskRight = vec_and(collisionMaskRight, *collisionMaskMiddle);
					
					//Finally - write out the collision data!
					*vDst = vec_sel(*vDst, zero, *collisionMaskMiddle);
					*vDst = vec_sel(*vDst, vCmpSrcLeft, collisionMaskLeft);
					*vDst = vec_sel(*vDst, vCmpSrcRight, collisionMaskRight);
					*vDst = vec_sel(*vDst, *vSrcUp, collisionMaskUp);
					*vDst = vec_sel(*vDst, *vSrcDown, collisionMaskDown);
				}
			}
			
			y = OSAtomicIncrement32Barrier(atomicY)-1;
		}
		
#ifdef FSJacobi_Error_Report
		[in_seq enterCriticalSection];
		float *error_list = (float*)&error;
		out_error[itrNo] += error_list[0] + error_list[1] + error_list[2] + error_list[3];
		[in_seq leaveCriticalSection];
#endif
		
		[in_seq waitForTask];
		
		FSStreamDesc *ptmp = p1;
		p1 = p2;
		p2 = ptmp;
		
	}
	
	//Finally apply the pressure
	y = OSAtomicIncrement32Barrier(atomicY)-1;
	for (; y<velocity.height; )
	{
		for (x=0; x<velocity.width; x++)
		{
			int xp1 = (x+1)%velocity.width;
			int yp1 = (y+1)%velocity.height;
			
			int xm1 = (x+velocity.width-1)%velocity.width;
			int ym1 = (y+velocity.height-1)%velocity.height;
			
			float *srcLeft = FSFloatPtrOffset(p2->data.f,
											  xm1*p2->strideX + y*p2->strideY);
			
			float *srcRight = FSFloatPtrOffset(p2->data.f,
											   xp1*p2->strideX + y*p2->strideY);
			
			float *srcUp = FSFloatPtrOffset(p2->data.f,
											x*p2->strideX + ym1*p2->strideY);
			
			float *srcDown = FSFloatPtrOffset(p2->data.f,
											  x*p2->strideX + yp1*p2->strideY);
			
			float *vDest = FSFloatPtrOffset(velocity.data.f,
											x*velocity.strideX
											+ y*velocity.strideY);
			
			vDest[0] -= (srcRight[0] - srcLeft[0]);
			vDest[1] -= (srcDown[0] - srcUp[0]);
		}
		y = OSAtomicIncrement32Barrier(atomicY)-1;
	}
	[in_seq waitForTask];
}
#endif


error *t_FS_JP_Name(int in_nIterations, mpTaskSet *in_seq,
						  int32_t *atomicY,
						  fluidStreamDesc velocity,
						  fluidStreamDesc collision,
						  fluidStreamDesc pressure[]
#ifdef FSJacobi_Error_Report
						,float out_error[]
#endif
						,int in_vectorize
						)
{
	//Make sure that all sizes are the same...
	if (velocity.width != collision.width
		|| velocity.width != pressure[0].width
		|| velocity.width != pressure[1].width
		|| velocity.height != collision.height
		|| velocity.height != pressure[0].height
		|| velocity.height != pressure[1].height)
	{
		return errorCreate(NULL, error_flags, "Jacobi: Fields of different size!");
	}
	
	//Other checks...
	if (velocity.type != FSCPU_Type_Float2D
		|| velocity.components != 2
		|| collision.type != FSCPU_Type_Character2D
		|| collision.components != 1
		|| pressure[0].type != FSCPU_Type_Float2D
		|| pressure[0].components != 1
		|| pressure[1].type != FSCPU_Type_Float2D
		|| pressure[1].components != 1)
	{
		return errorCreate(NULL, error_flags, "Jacobi: type/component mismatch!");
	}

#ifdef __APPLE_ALTIVEC__
	//Is it safe to use the "Velocity Engine"
	if (in_vectorize &&
		((int)velocity.data.f)%16 == 0 &&
		((int)pressure[0].data.f)%16 == 0 && ((int)pressure[1].data.f)%16 == 0
		&& ((int)pressure[2].data.f)%16 == 0
		&& ((int)pressure[3].data.f)%16 == 0
		&& ((int)pressure[4].data.f)%16 == 0
		&& velocity.width%4 == 0 && velocity.strideX == 8
		&& pressure[0].strideX == 4 && pressure[1].strideX == 4
		&& pressure[2].strideX == 4 && pressure[3].strideX == 4
		&& pressure[3].strideX == 4)
	{
		t_FS_JP_ALTIVEC_Name(in_nIterations, in_seq, atomicY,
								velocity, collision, pressure
#ifdef FSJacobi_Error_Report
						,out_error
#endif
								);
		return;
	}
#endif

	fluidStreamDesc *p1 = pressure;
	fluidStreamDesc *p2 = pressure+1;

#ifdef FSJacobi_Error_Report
	memset(out_error, 0, sizeof(float)*in_nIterations);
	float f_error;
#endif

	//Good, now we iterate...
	int itrNo,x,y;
	for (itrNo =0; itrNo < in_nIterations; itrNo++)
	{
		y = OSAtomicIncrement32Barrier(atomicY)-1;
#ifdef FSJacobi_Error_Report
		f_error = 0;
#endif
		
		for (; y<velocity.height;)
		{
			int yp1 = (y+1)%velocity.height;
			int ym1 = (y+velocity.height-1)%velocity.height;
			
			int xm1 = velocity.width-1;
			
			int colStrideY = y*collision.strideY;
			int colStrideX = 0;
			
			int p1StrideY = y*p1->strideY;
			int p2StrideY = y*p2->strideY;
			
			int p2StrideYP1 = yp1*p2->strideY;
			int p2StrideYM1 = ym1*p2->strideY;
			
			int vStrideY = y*velocity.strideY;
			int vStrideYP1 = yp1*velocity.strideY;
			int vStrideYM1 = ym1*velocity.strideY;
			
			int p2StrideX = 0;
			int p2StrideXM1 = xm1*p2->strideX;
			
			int vStrideX = 0;
			int vStrideXM1 = xm1*velocity.strideX;
			
			int vStrideXP1 = 0;
			int p2StrideXP1 = 0;
			int p1StrideX = 0;
			
			float srcLeftVal = *(FSFloatPtrOffset(p2->data.f,
												p2StrideXM1 + p2StrideY));
			
			float srcVal = *(FSFloatPtrOffset(p2->data.f,
												 p2StrideX + p2StrideY));
			
			
			float vLeftVal = *(FSFloatPtrOffset(velocity.data.f,
											vStrideXM1
											+ vStrideY));
			
			float vVal = *(FSFloatPtrOffset(velocity.data.f,
												vStrideX
												+ vStrideY));
			for(x=0; x<velocity.width; x++)
			{
				//NOTE: Under PPC - Int division/mod very expensive.  Use
				//		?: instead!
				int xp1;
				if (x+1<velocity.width)
				{
					xp1 = x+1;
					vStrideXP1 += velocity.strideX;
					p2StrideXP1 +=p2->strideX;
				}
				else
				{
					xp1 = 0;
					vStrideXP1 = 0;
					p2StrideXP1 = 0;
				}
				
				unsigned char ptrCollision = collision.data.c[colStrideX
															+ colStrideY];
				colStrideX += collision.strideX;
				
				float *dest = FSFloatPtrOffset(p1->data.f,
											   p1StrideX + p1StrideY);
				p1StrideX += p1->strideX;
				
				float srcRightVal = *(FSFloatPtrOffset(p2->data.f,
												  p2StrideXP1 + p2StrideY));
				
				float *srcUp = FSFloatPtrOffset(p2->data.f,
												  p2StrideX + p2StrideYM1);
				
				float *srcDown = FSFloatPtrOffset(p2->data.f,
												  p2StrideX + p2StrideYP1);
				
				float vRightVal = *(FSFloatPtrOffset(velocity.data.f,
												vStrideXP1
												+ vStrideY));
				
				float *vUp = FSFloatPtrOffset(velocity.data.f,
												vStrideX
												+ vStrideYM1);
				
				float *vDown = FSFloatPtrOffset(velocity.data.f,
												vStrideX
												+ vStrideYP1);
				
				//No collisions - regular pressure EQ
				if (ptrCollision == 0)
				{

					float newVal = (srcLeftVal + srcRightVal + *srcUp + *srcDown
									- (vRightVal - vLeftVal + vDown[1] - vUp[1]))*0.25f;
					
#ifdef FSJacobi_Error_Report
					f_error += fabsf(newVal - srcVal);
#endif

#ifdef FSJacobi_Field
					*dest = 0.9f*newVal +  0.1f*srcVal;
#else
					*dest = newVal;
#endif
				}
				
				//Collisions - compensate
				else
				{
					unsigned char left, right, up, down;
					
					left = collision.data.c[xm1*collision.strideX
											+ y*collision.strideY];
					right = collision.data.c[xp1*collision.strideX
											 + y*collision.strideY];
					up = collision.data.c[x*collision.strideX
										  + ym1*collision.strideY];
					down = collision.data.c[x*collision.strideX
											+ yp1*collision.strideY];
					
					if (left == 0)
					{
						*dest = srcLeftVal;
					}
					else if (right == 0)
					{
						*dest = srcRightVal;
					}
					else if (up == 0)
					{
						*dest = *srcUp;
					}
					else if (down == 0)
					{
						*dest = *srcDown;
					}
					else
						*dest = 0;
				}
				
				
				xm1 = x;
				
				srcLeftVal = srcVal;
				srcVal = srcRightVal;
				
				vLeftVal = vVal;
				vVal = vRightVal;
				
				p2StrideXM1 = p2StrideX;
				p2StrideX = p2StrideXP1;
				
				vStrideXM1 = vStrideX;
				vStrideX = vStrideXP1;
			}
			
			y = OSAtomicIncrement32Barrier(atomicY)-1;
		}
		
#ifdef FSJacobi_Error_Report
		mpTaskSetEnterCriticalSection(in_seq);
		out_error[itrNo] += f_error;
		mpTaskSetLeaveCriticalSection(in_seq);
#endif
		
		mpTaskSetSync(in_seq);
	
		
		fluidStreamDesc *ptmp = p1;
		p1 = p2;
		p2 = ptmp;
		
	}

	//Finally apply the pressure
	y = OSAtomicIncrement32Barrier(atomicY)-1;
	for (; y<velocity.height; )
	{
		for (x=0; x<velocity.width; x++)
		{
			int xp1 = (x+1)%velocity.width;
			int yp1 = (y+1)%velocity.height;
			
			int xm1 = (x+velocity.width-1)%velocity.width;
			int ym1 = (y+velocity.height-1)%velocity.height;
			
			float *srcLeft = FSFloatPtrOffset(p2->data.f,
											  xm1*p2->strideX + y*p2->strideY);
			
			float *srcRight = FSFloatPtrOffset(p2->data.f,
											   xp1*p2->strideX + y*p2->strideY);
			
			float *srcUp = FSFloatPtrOffset(p2->data.f,
											x*p2->strideX + ym1*p2->strideY);
			
			float *srcDown = FSFloatPtrOffset(p2->data.f,
											  x*p2->strideX + yp1*p2->strideY);
			
			float *vDest = FSFloatPtrOffset(velocity.data.f,
											x*velocity.strideX
											+ y*velocity.strideY);
			
			vDest[0] -= (srcRight[0] - srcLeft[0]);
			vDest[1] -= (srcDown[0] - srcUp[0]);
		}
		y = OSAtomicIncrement32Barrier(atomicY)-1;
	}
	mpTaskSetSync(in_seq);
	
	return NULL;
}



error *t_FS_JD_Name(int in_nIterations,
						   mpTaskSet *in_seq, int32_t *atomicY,
						   fluidStreamDesc *v1, fluidStreamDesc *v2,
						   fluidStreamDesc *collision,
						   float viscocity, float timestep
#ifdef FSJacobi_Error_Report
							,float out_error[]
#endif
						)
{
	float alpha = 1.0f/viscocity * timestep;
	float beta = 1.0f/(alpha + 4);
	
	//Validate dimensions
	if (v1->width != v2->width ||
		v1->width != collision->width ||
		v1->height != v2->height ||
		v1->height != collision->height)
	{
		return errorCreate(NULL, error_flags, "Jacobi: Sizes differ");
	}
	
	//Validate the rest...
	if (v1->type != FSCPU_Type_Float2D
		|| v2->type != FSCPU_Type_Float2D
		|| collision->type != FSCPU_Type_Character2D
		|| v1->components != 2
		|| v2->components != 2
		|| collision->components != 1)
	{
		return errorCreate(NULL, error_flags, "Jacobi: Wrong data types");
	}
	
#ifdef FSJacobi_Error_Report
	memset(out_error, 0, sizeof(float)*in_nIterations);
	float f_error;
#endif
	
	//Now loop...
	int itrNo,x,y;
	for (itrNo =0; itrNo < in_nIterations; itrNo++)
	{
		y = OSAtomicIncrement32Barrier(atomicY)-1;
#ifdef FSJacobi_Error_Report
		f_error = 0;
#endif
		
		for (; y<v1->height;)
		{
			int colStrideY = y*collision->strideY;
			int colStrideX = 0;
			
			int xm1 = v1->width-1;
			int ym1 = (y+v1->height-1)%v1->height;
			int yp1 = (y+1)%v1->height;
			
			int curStrideX2 = 0;
			int prevStrideX2 = xm1*v2->strideX;
			
			int v1StrideY = y*v1->strideY;
			int v2StrideY = y*v2->strideY;
			int v2StrideYP1 = yp1*v2->strideY;
			int v2StrideYM1 = ym1*v2->strideY;
			
			int v1StrideX = 0;
			
			int nextStrideX2 = 0;
			
			float *v2Left = FSFloatPtrOffset(v2->data.f,
												 prevStrideX2
												 + v2StrideY);
			
			for(x=0; x<v1->width; x++)
			{
				//Base: 10fps, DT=0.1, res=256, c2d.
				//	- Incrementally compute xm1: 11fps, DT=0.08
				//	- Moving y out (normal no change - compiler caught it!)
				//	- Did some for pressure: 11fps, DT=0.083
				//	- Removed all integer mults in inner loop: 14fps, DT=0.06
				
				//NOTE: Under PPC - Int division/mod very expensive.  Use
				//		?: instead!
				int xp1;
				if (x+1<v1->width)
				{
					xp1 = x+1;
					nextStrideX2 += v2->strideX;
				}
				else
				{
					xp1 = 0;
					nextStrideX2 = 0;
				}
				
				unsigned char collisionDat = collision->data.c[colStrideX
															   + colStrideY];
				colStrideX += collision->strideX;
				
				float *dest = FSFloatPtrOffset(v1->data.f,
											   v1StrideX
												+ v1StrideY);
				v1StrideX += v1->strideX;
				
				float *v2loc = FSFloatPtrOffset(v2->data.f,
												curStrideX2
												+ v2StrideY);
				
				float *v2Right = FSFloatPtrOffset(v2->data.f,
												  nextStrideX2
												  + v2StrideY);
				
				float *v2Up = FSFloatPtrOffset(v2->data.f,
											   curStrideX2
											   + v2StrideYM1);
				
				float *v2Down = FSFloatPtrOffset(v2->data.f,
												 curStrideX2
												 + v2StrideYP1);
				
				//Branch has next to no effect on performance...
				if (collisionDat == 0)
				{
					dest[0] = (v2Left[0] + v2Right[0] + v2Up[0] + v2Down[0]
								+ v2loc[0]*alpha)*beta;
					
					dest[1] = (v2Left[1] + v2Right[1] + v2Up[1] + v2Down[1]
								+ v2loc[1]*alpha)*beta;

#ifdef FSJacobi_Error_Report
					
					float *src = FSFloatPtrOffset(v2->data.f,
												   v1StrideX
												   + v1StrideY);
					f_error += fabsf(dest[0] - src[0])
							+ fabsf(dest[1] - src[1]);
#endif
				}
				else
				{
					unsigned char left, right, up, down;
					
					left = collision->data.c[xm1*collision->strideX
											+ y*collision->strideY];
					right = collision->data.c[xp1*collision->strideX
											 + y*collision->strideY];
					up = collision->data.c[x*collision->strideX
										  + ym1*collision->strideY];
					down = collision->data.c[x*collision->strideX
											+ yp1*collision->strideY];
					
					if (left == 0)
					{
						dest[0] = -v2Left[0];
						dest[1] = -v2Left[1];
					}
					else if (right == 0)
					{
						dest[0] = -v2Right[0];
						dest[1] = -v2Right[1];
					}
					else if (up == 0)
					{
						dest[0] = -v2Up[0];
						dest[1] = -v2Up[1];
					}
					else if (down == 0)
					{
						dest[0] = -v2Down[0];
						dest[1] = -v2Down[1];
					}
					else
					{
						dest[0] = 0;
						dest[1] = 0;
					}
				}
				
				xm1 = x;
				
				prevStrideX2 = curStrideX2;
				curStrideX2 = nextStrideX2;
				
				v2Left = v2loc;
			}
			y = OSAtomicIncrement32Barrier(atomicY)-1;
		}
		
#ifdef FSJacobi_Error_Report
		mpTaskSetEnterCriticalSection(in_seq);
		out_error[itrNo] += f_error;
		mpTaskSetLeaveCriticalSection(in_seq);
#endif
		
		mpTaskSetSync(in_seq);
		fluidStreamDesc *tmp = v1;
		v1 = v2;
		v2 = tmp;
	}
	
	return NULL;
}


