//Computes the viscosity... yet again!
kernel void main(	read_only image2d_t in_velX,
					write_only image2d_t out_velX,
					read_only image2d_t in_velY,
					write_only image2d_t out_velY,
					float alpha,
					float beta)
{
	int2 p = (int2)(get_global_id(0), get_global_id(1));
	
	int2 shiftX = (int2)(1,0);
	int2 shiftY = (int2)(0,1);
	
	sampler_t s =	CLK_FILTER_NEAREST |
					CLK_ADDRESS_NONE |
					CLK_NORMALIZED_COORDS_FALSE;
	
	//All hail swizzles?!  Actually, reformatting the data seems like the
	//best course of action...
	
	//General case first...
	float4 curVel = read_imagef(in_velX, s, p);
	
	//Top/bottom (simply negate)
	float4 prevVel, nextVel;
	if (p.y == 0)
		prevVel = -curVel;
	else
		prevVel = read_imagef(in_velX, s, p-shiftY);
	
	if (p.y == get_image_height(in_velX)-1)
		nextVel = -curVel;
	else
		nextVel = read_imagef(in_velX, s, p+shiftY);
	
	//Left and right are special...
	float4 leftVel, rightVel;
	if (p.x == 0)
		leftVel.x = -curVel.x;
	else
		leftVel.x = read_imagef(in_velX, s, p-shiftX).w;
	
	if (p.x == get_image_width(in_velX)-1)
		rightVel.w = -curVel.w;
	else
		rightVel.w = read_imagef(in_velX, s, p+shiftX).x;
	
	//Assume compiler is somewhat smart...
	leftVel.yzw = curVel.xyz;
	rightVel.xyz = curVel.yzw;
	
	//And do the kernel...
	curVel = (curVel*alpha + prevVel + nextVel + leftVel + rightVel)*beta;
	write_imagef(out_velX, p, curVel);
	
	
	//All hail swizzles?!  Actually, reformatting the data seems like the
	//best course of action...
	
	//General case first...
	curVel = read_imagef(in_velY, s, p);
	
	//Top/bottom (simply negate)
	if (p.y == 0)
		prevVel = -curVel;
	else
		prevVel = read_imagef(in_velY, s, p-shiftY);
	
	if (p.y == get_image_height(in_velY)-1)
		nextVel = -curVel;
	else
		nextVel = read_imagef(in_velY, s, p+shiftY);
	
	//Left and right are special...
	if (p.x == 0)
		leftVel.x = -curVel.x;
	else
		leftVel.x = read_imagef(in_velY, s, p-shiftX).w;
	
	if (p.x == get_image_width(in_velY)-1)
		rightVel.w = -curVel.w;
	else
		rightVel.w = read_imagef(in_velY, s, p+shiftX).x;
	
	//Assume compiler is somewhat smart...
	leftVel.yzw = curVel.xyz;
	rightVel.xyz = curVel.yzw;
	
	//And do the kernel...
	curVel = (curVel*alpha + prevVel + nextVel + leftVel + rightVel)*beta;
	write_imagef(out_velY, p, curVel);
}
