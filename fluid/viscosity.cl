//Computes the viscosity... yet again!
kernel void main(	read_only image2d_t in_velX,
					read_only image2d_t in_velY,
					write_only image2d_t out_velX,
					write_only image2d_t out_velY)
{
	int2 p = (int2)(get_global_id(0), get_global_id(1));
	
	int2 shiftX = (int2)(1,0);
	int2 shiftY = (int2)(0,1);
	
	sampler_t = CLK_FILTER_NEAREST | CLK_ADDRESS_NONE | CLK_NORMALIZED_COORDS_FALSE;
	
	//All hail swizzles?!  Actually, reformatting the data seems like the
	//best course of action...
}
