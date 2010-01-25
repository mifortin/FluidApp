//CL implementation of pressure
kernel void main(	read_only image2d_t	in_pressure,
					read_only image2d_t in_velX,
					read_only image2d_t in_velY,
					write_only image2d_t out_pressure)
{
	int2 p = (int2)(get_global_id(0), get_global_id(1));
	
	int2 shiftX = (int2)(1,0);
	int2 shiftY = (int2)(0,1);
	
	sampler_t s	=	CLK_FILTER_NEAREST |
					CLK_ADDRESS_NONE |
					CLK_NORMALIZED_COORDS_FALSE;
	
	//General case first...
	float4 curVelX = read_imagef(in_velX, s, p);
	float4 curVelY = read_imagef(in_velY, s, p);
	float4 curPressure = read_imagef(in_pressure, s, p);
	
	//Top/bottom (simply negate)
	float4 prevVelY, nextVelY;
	float4 prevPressure, nextPressure;
	if (p.y == 0)
	{
		prevVelY = -curVelY;
		prevPressure = curPressure;
	}	
	else
	{
		prevVelY = read_imagef(in_velY, s, p-shiftY);
		prevPressure = read_imagef(in_pressure, s, p-shiftY);
	}
	
	if (p.y == get_image_height(in_velX)-1)
	{
		nextVelY = -curVelY;
		nextPressure = curPressure;
	}
	else
	{
		nextVelY = read_imagef(in_velY, s, p+shiftY);
		nextPressure = read_imagef(in_pressure, s, p+shiftY);
	}
	
	float4 leftVelX, rightVelX;
	float4 leftPressure, rightPressure;
	if (p.x == 0)
	{
		leftVelX.x = -curVelX.x;
		leftPressure.x = curPressure.x;
	}
	else
	{
		leftVelX.x = read_imagef(in_velX, s, p-shiftX).w;
		leftPressure.x = read_imagef(in_pressure, s, p-shiftX).w;
	}
	
	if (p.x == get_image_width(in_velX)-1)
	{
		rightVelX.w = -curVelX.w;
		rightPressure.w = curPressure.w;
	}
	else
	{
		rightVelX.w = read_imagef(in_velX, s, p+shiftX).x;
		rightPressure.w = read_imagef(in_pressure, s, p+shiftX).x;
	}
	
	leftVelX.yzw = curVelX.xyz;
	leftPressure.yzw = curPressure.xyz;
	
	rightVelX.xyz = curVelX.yzw;
	rightPressure.xyz = curPressure.yzw;
	
	//Finally the math...
	curPressure = (leftPressure + rightPressure + nextPressure + prevPressure
					- (rightVelX - leftVelX + nextVelY - prevVelY)) * 0.25f;
	
	//Don't forget to write it all out!
	write_imagef(out_pressure, p, curPressure);
}
