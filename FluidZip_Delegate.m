//
//  FluidZip_Delegate.m
//  FluidApp
//

#import "FluidZip_Delegate.h"


@implementation FluidZip_Delegate

- (void)awakeFromNib
{
	r_windows = [[NSMutableArray alloc] initWithCapacity:10];
}

- (void)dealloc
{
	[r_windows release];
	[super dealloc];
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
	NSString *extension = [filename pathExtension];
	if ([extension isEqualToString:@"jpg"]
		|| [extension isEqualToString:@"jpeg"]
		|| [extension isEqualToString:@"png"]
		|| [extension isEqualToString:@"vec"]
		|| [extension isEqualToString:@"tga"])
	{
		NSImage *i = nil;
		if ([[filename pathExtension] compare:@"vec"] == NSOrderedSame)
		{
			//Put in decompression code here...
			NSSize s = {640,480};
			i = [[NSImage alloc] initWithSize:s];
		}
		else
		{
			i = [[NSImage alloc] initWithContentsOfFile:filename];
		}
		
		[NSBundle loadNibNamed:@"FluidZip_Window" owner:self];
		[r_windows addObject:i_window];
		[i_window release];
		
		[i_window setImage:i];
		[i_window setTitle:[filename lastPathComponent]];
		[i_window setDelegate:self];
		
		[i release];
		
		return YES;
	}
	
	return NO;
}

- (IBAction)onMenuFileOpen:(id)in_src
{
	NSOpenPanel *nsp = [NSOpenPanel openPanel];
	
	[nsp setCanChooseFiles:YES];
	[nsp setCanChooseDirectories:NO];
	[nsp setResolvesAliases:YES];
	[nsp setAllowsMultipleSelection:NO];
	[nsp setTitle:@"Open Image"];
	
	if ([nsp runModalForTypes:[NSArray arrayWithObjects:
										@"jpg",@"jpeg",@"png",@"vec",@"tga",
										nil]] == NSOKButton)
	{
		[[NSDocumentController sharedDocumentController]
		 noteNewRecentDocumentURL:[nsp URL]];
		
		[self application:NSApp openFile:[nsp filename]];
	}
}


unsigned char clamp(float f)
{
	if (f < 0)
		return 0;
	if (f > 255)
		return 255;
	
	return (unsigned char)f;
}


void decompressRow(int w, int stride, int *tmp, unsigned char *out_d,
								int **io_indices, int **io_data)
{
	int *indices = *io_indices;
	int *data = *io_data;
	
	//Decompress some uchar data...
	int s = data[0];
	int e = data[1];
	
	int si = 0;
	
	int i;
	int c;
	for (i=0; i<indices[0]; i++)
	{
		int se = indices[i+1];
		int ed = data[i+2];
		
		for (c=si; c<se; c++)
		{
			out_d[c*stride] = (((se - c) * s + (c - si) * ed) / (se-si));
		}
		
		si = se;
		s = ed;
	}
	
	for (c=si; c<w; c++)
	{
		out_d[c*stride] = (((w - c) * s + (c - si) * e) / (w-si));
	}
	
	*io_data += 2 + indices[0];
	*io_indices += 1 + indices[0];
}


void compressRow(int w, int s, int *tmp, unsigned char *in_d,
								int **io_indices, int **io_data,
								int *out_indices, int *out_data)
{
	//Compress some uchar data...
	int *indices = *io_indices;
	int *data = *io_data;
	
	int x;
	for (x=1; x<w-1; x++)
	{
		tmp[x] = (int)in_d[(x-1)*s] - (int)in_d[(x+1)*s];
	}
	tmp[0] = tmp[1];
	tmp[w-1] = tmp[w-2];
	
	data[0] = in_d[0];
	data[1] = in_d[(w-1)*s];
	
	indices[0] = 0;
	
	int curv = 0;
	int prevRow = 0;
	for (x=1; x<w-1; x++)
	{
		curv = (int)tmp[x-1] - (int)tmp[x+1];
		
		if (abs(curv) > 4)
		{
			curv = (int)tmp[x-1] - (int)tmp[x+1];
			
			data[2+indices[0]] = in_d[x*s];
			indices[0]++;
			indices[indices[0]] = x;
		}
		if (x > 1 && abs(curv) > 4)
			prevRow = 1;
		else
			prevRow = 0;
	}
	
	
	*out_indices = 1+indices[0];
	*out_data = 2+indices[0];
	
	*io_data += 2 + indices[0];
	*io_indices += 1 + indices[0];
}


- (IBAction)onMenuFormatImageCompress:(id)in_src
{
	if (m_focus == nil)		return;
	
	NSBitmapImageRep *bir = [NSBitmapImageRep alloc];
	[[m_focus image] lockFocus];
	[bir initWithFocusedViewRect:NSMakeRect(0, 0, [[m_focus image] size].width,
											[[m_focus image] size].height)];
	[[m_focus image] unlockFocus];
	
	
	NSBitmapImageRep *cpy = [[NSBitmapImageRep alloc]
		initWithBitmapDataPlanes:NULL pixelsWide:[[m_focus image] size].width
									pixelsHigh:[[m_focus image] size].height
						bitsPerSample:[bir bitsPerPixel]/[bir samplesPerPixel]
									samplesPerPixel:[bir samplesPerPixel]
									hasAlpha:[bir hasAlpha] isPlanar:NO
									colorSpaceName:NSCalibratedRGBColorSpace
									bytesPerRow:[bir bytesPerRow]
							 bitsPerPixel:[bir bitsPerPixel]];
	
	NSImage *i2 = [[NSImage alloc] initWithSize:[[m_focus image] size]];
	[i2 addRepresentation:cpy];
	
	//memcpy([cpy bitmapData], [bir bitmapData],
	//	   [bir bytesPerRow]*[i2 size].height);
	
	unsigned char *s = [bir bitmapData];
	unsigned char *d = [cpy bitmapData];
	
	//Now - apply a basic compression kernel (just to see results)
	int x,y,z;
	int w = [i2 size].width, h = [i2 size].height, bpp = [bir bitsPerPixel]/8;
	
	printf("Uncompressed image size: %ikb (32bit: %ikb)\n", w*h*bpp/1024, w*h*4*bpp/1024);
	int *indices = malloc(sizeof(int)*w*h);
	float *data = malloc(sizeof(float)*w*h*bpp);
	int *iData = malloc(sizeof(int)*w*h*bpp);
	
	float *deriv = malloc(sizeof(float)*w*h*bpp);
	float *curv = malloc(sizeof(float)*w*h*bpp);
	
	
	//First, clean up the input?
//	for (y=0; y<h; y++)
//	{
//		for (x=0; x<w; x++)
//		{
//			for (z=0; z<bpp; z++)
//			{
//				if
//			}
//		}
//	}
	
	
	//First, compute the curvature...
	
	//Loop through the data accumulating curvature.  Once curvature change
	//exceeds a given threshold (on any component) create an entry.
	int memUsageReal = 0;
	int mem8bit = 0;
	for (y=0; y<h; y++)
	{
		//Clean up a bit
//		for (x=1; x<w-1; x++)
//		{
//			float ds = 0;
//			for (z=0; z<bpp; z++)
//			{
//				if (z != 3)
//				{
//					float diff = ((float)s[z + (x+1)*bpp + y*bpp*w] - 
//												 (float)s[z + (x-1)*bpp + y*bpp*w]);
//					ds += diff*diff;
//				}
//			}
//			ds = sqrtf(ds);
//			
//			if (ds < 1.0f)
//			{
//				for (z=0; z<bpp; z++)
//				{
//					s[z + (x+0)*bpp + y*bpp*w]
//						= (s[z + (x-1)*bpp + y*bpp*w] + s[z + (x+1)*bpp + y*bpp*w])/2;
//				}
//			}
//		}
		
		//Take derivatives
		for (x=0; x<w; x++)
		{
			deriv[x + y*w] = 0;
			for (z=0; z<bpp; z++)
			{
				if (x== 0 || x == w-1)
					curv[x + y*w] = 0;
				else if (z != 3)
				{
					if (y == 0)
					{
						float diff = ((float)s[z + (x+0)*bpp + y*bpp*w] - 
													 (float)s[z + (x-1)*bpp + y*bpp*w]);
						deriv[x + y*w] += diff*diff;
					}
					else
					{
						float dy1 = (float)s[z + (x+0)*bpp + y*bpp*w] -
									(float)d[z + (x+0)*bpp + (y-1)*bpp*w];
						float dy2 = (float)s[z + (x-1)*bpp + y*bpp*w] -
									(float)d[z + (x-1)*bpp + (y-1)*bpp*w];
						
						if (fabs(dy1) < 6) dy1 = 0;
						if (fabs(dy2) < 6) dy2 = 0;
						float diff = (dy1 - dy2);
						deriv[x + y*w] += diff*diff;
					}
				}
			}
			deriv[x + y*w] = sqrtf(deriv[x + y*w]);
		}
		
		for (x=w-2; x>=1; x--)
		{
			curv[x + y*w] = (deriv[(x+0) + y*w]);
		}
		
		//Loop through and compress a single line
		float cmp[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		indices[y*w] = 0;
		for (z=0; z<bpp; z++)
		{
			if (y == 0)
				data[y*bpp*w + z] = s[y*bpp*w + z];
			else
				data[y*bpp*w + z] = s[y*bpp*w + z] - d[(y-1)*bpp*w + z];
		}
		
		memUsageReal += sizeof(int) + bpp*sizeof(float);
		mem8bit += sizeof(short) + sizeof(char)*bpp;
		
		int lastNeedToWork = 0;
		int skippedWork = 0;
		int justDidWork = 0;
		int px = 0;
		
		for (x=1; x<w; x++)
		{
			int needToWork = 0;
			float simpleSum = 0;
			
			cmp[0] += (curv[x + y*w]);
			
			if (x < w-1)
				simpleSum += abs(curv[(x+1) + y*w])
						+ abs(curv[(x-1) + y*w]);
			
			if (abs(cmp[0]) > 4.0f)
			{
				needToWork = 1;
			}
			
			if (needToWork == 0 && justDidWork > 1)
				justDidWork--;
			else if (justDidWork == 1 && needToWork == 0)
			{
				justDidWork = -1;
				needToWork = 1;
			}
			
			if (skippedWork)
				needToWork = 1;
			
			if (x == w-1)	needToWork = 1;
			else
			{
				if (x < lastNeedToWork + 0  && needToWork)
				{
					needToWork = 0;
				
					skippedWork = 1;
				}
				else if (needToWork)
					skippedWork = 0;
			}
			
			//if (px >= 255)		needToWork = 1;
			if (needToWork)		lastNeedToWork = x;
			
			if (needToWork)
			{
				int nx = (x == w-1)?x:x-1;
				
				indices[y*w]++;
				int ci = indices[y*w];
				
				indices[y*w+ci] = nx - px;
				px = nx;
				for (z=0; z<bpp; z++)
				{
					if (y == 0)
						data[ci*bpp + y*bpp*w + z] = s[nx*bpp + y*bpp*w + z];
					else
						data[ci*bpp + y*bpp*w + z] = s[nx*bpp + y*bpp*w + z]
													-d[nx*bpp + (y-1)*bpp*w + z];
						
					cmp[z] = 0;
				}
				
				memUsageReal += sizeof(int) + bpp*sizeof(float);
				mem8bit += sizeof(short) + bpp;
				
				/*if (justDidWork == -1)
					justDidWork = 0;
				else
					justDidWork = 3;*/
			}
		}
		
		//Pre-compute results...
		int sc = indices[y*w];
		int prevX = 0;
		
		for (z=0; z<bpp; z++)
			d[z + y*bpp*w] = data[z+y*bpp*w];
		
		for (x=1; x<=sc; x++)
		{
			int curX = indices[y*w+x] + prevX;
			
			int x2;
			for (x2=prevX; x2<=curX; x2++)
			{
				float scaleF = (float)(x2-prevX)/(float)(curX - prevX);
				
				for(z=0;z<bpp;z++)
				{
					if (y == 0)
						d[z + x2*bpp + y*bpp*w] =
								(scaleF)*data[x*bpp + y*bpp*w + z]
								+ (1-scaleF)*data[(x-1)*bpp + y*bpp*w + z];
					else
						d[z + x2*bpp + y*bpp*w] =
								clamp((scaleF)*data[x*bpp + y*bpp*w + z]
								+ (1-scaleF)*data[(x-1)*bpp + y*bpp*w + z]
								+ d[z +x2*bpp + (y-1)*bpp*w]);
					
					if (x2 == prevX && z != 3)
					{
						//d[z + x2*bpp + y*bpp*w] = 0;
					}
				}
			}
			
			prevX = curX;
		}
		
	}
	printf("Compressed 32-bit: %ikb\nPotential 16-bit: %ikb\nPotential 8-bit: %ikb\n",
				memUsageReal/1024, memUsageReal/2048, mem8bit/1024);
	
	//Display the results...
	for (y=0; y<h; y++)
	{
		int sc = indices[y*w];
		int prevX = 0;
		
		for (z=0; z<bpp; z++)
			d[z + y*bpp*w] = data[z+y*bpp*w];
		
		for (x=1; x<=sc; x++)
		{
			int curX = indices[y*w+x] + prevX;
			
			int x2;
			for (x2=prevX; x2<=curX; x2++)
			{
				//float scaleF = (float)(x2-prevX)/(float)(curX - prevX);
				
				for(z=0;z<bpp;z++)
				{
					//if (y == 0)
//						d[z + x2*bpp + y*bpp*w] =
//						(scaleF)*data[x*bpp + y*bpp*w + z]
//						+ (1-scaleF)*data[(x-1)*bpp + y*bpp*w + z];
//					else
//						d[z + x2*bpp + y*bpp*w] =
//						(scaleF)*data[x*bpp + y*bpp*w + z]
//						+ (1-scaleF)*data[(x-1)*bpp + y*bpp*w + z]
//						+ d[z +x2*bpp + (y-1)*bpp*w];
					
					if (x2 == prevX && z != 3)
					{
					//	d[z + x2*bpp + y*bpp*w] = 0;
					}
					else
					{
					//	d[z + x2*bpp + y*bpp*w] = 255;
					}
				}
			}
			
			prevX = curX;
		}
	}
	
	//Display the results...
//	for (y=0; y<h; y++)
//	{
//		for (x=0; x<w; x++)
//		{
//			for (z=0; z<bpp; z++)
//			{
//				d[z + x*bpp + y*bpp*w] = curv[x+y*w];
//				
//				if (z == 3)
//					d[z + x*bpp + y*bpp*w] = 255;
//			}
//		}
//	}
	
	/*for (y=0; y<h; y++)
	{
		for(x=0; x<w; x++)
		{
			for (z=0; z<bpp; z++)
			{
				if (x == 0)
					d[z + x*bpp + y*bpp*w] = s[z + x*bpp + y*bpp*w];
				else
					d[z + x*bpp + y*bpp*w] = (s[z + x*bpp + y*bpp*w] - 
											 s[z + (x-1)*bpp + y*bpp*w]) + 127;
			}
		}
	}
	
	for (y=0; y<h; y++)
	{
		for(x=w-1; x>=1; x--)
		{
			for (z=0; z<bpp; z++)
			{
				if (z == 1)
					d[z + x*bpp + y*bpp*w] = (d[z + x*bpp + y*bpp*w] - 
									d[z + (x-1)*bpp + y*bpp*w]) + 127;
				else
					d[z + x*bpp + y*bpp*w] = 0;
			}
		}
	}*/
	
	float c[6] = {0,0,0,0,0,0};
	
	int *dc = iData;
	int *ic = indices;
	
	int *dce = iData;
	int *ice = indices;
	
	unsigned char *yuv = malloc(w*h*3);
	
	int totId = 0, totDa = 0;
	
	int diff[9] = {0};
	
	for (y=0; y<h; y++)
	{
		
		for (x=0; x<w; x++)
		{
			unsigned char Y = 
					clamp
						(	0.299f*(float)s[0+x*bpp+y*bpp*w]
						+	0.587f*(float)s[1+x*bpp+y*bpp*w]
						+	0.144f*(float)s[2+x*bpp+y*bpp*w]);
			unsigned char Pb =  
					clamp
						(	-0.14713*0.5f/0.436f*s[0+x*bpp+y*bpp*w]
						-	0.28886*0.5f/0.436f*s[1+x*bpp+y*bpp*w]
						+	0.436*0.5f/0.436f*s[2+x*bpp+y*bpp*w]
						+	128.0f);
			unsigned char Pr = 
					clamp
						(	0.601f*0.5f/0.436f*s[0+x*bpp+y*bpp*w]
						-	0.51499f*0.5f/0.436f*s[1+x*bpp+y*bpp*w]
						-	0.10001f*0.5f/0.436f*s[2+x*bpp+y*bpp*w]
						+	128.0f);
			
//			d[0+x*bpp + y*bpp*w] = clamp(Y + ((float)Pr-128.0f)*1.13983f*0.615f/0.5f);
//			d[1+x*bpp + y*bpp*w] = clamp(Y + ((float)Pb-128.0f)*(-0.39465f)*0.436f/0.5f
//											+((float)Pr-128.0f)*(-0.58060f)*0.436f/0.5f);
//			d[2+x*bpp + y*bpp*w] = clamp(Y + ((float)Pb-128.0f)*2.03211f*0.436f/0.5f);

			yuv[x*3+y*w*3+0] = Y;
			yuv[x*3+y*w*3+1] = Pb;
			yuv[x*3+y*w*3+2] = Pr;
			
//			d[0+x*bpp + y*bpp*w] = 0;
//			d[1+x*bpp + y*bpp*w] = Pb;
//			d[2+x*bpp + y*bpp*w] = Pr;

			//d[x*bpp + y*bpp*w] = 0;
		}
		
		for (x=0; x<w; x++)
		{
			if (x!= 0 && x != w-1)
			{
				deriv[0+x*6] = s[0+(x+1)*bpp+y*bpp*w]-s[0+(x-1)*bpp+y*bpp*w];
				deriv[1+x*6] = s[1+(x+1)*bpp+y*bpp*w]-s[1+(x-1)*bpp+y*bpp*w];
				deriv[2+x*6] = s[2+(x+1)*bpp+y*bpp*w]-s[2+(x-1)*bpp+y*bpp*w];
				deriv[3+x*6] = yuv[(x+1)*3+0] - yuv[(x-1)*3+0];
				deriv[4+x*6] = yuv[(x+1)*3+1] - yuv[(x-1)*3+1];
				deriv[5+x*6] = yuv[(x+1)*3+2] - yuv[(x-1)*3+2];
			}
			else
			{
				deriv[x] = 0;
			}
		}
		
		for (x=1; x<w-1; x++)
		{
			for (z=0; z<6; z++)
			{
				c[z] += fabs(deriv[z+(x+1)*6] - deriv[z+(x-1)*6]);
			}
		}

		int countId=0, countDa=0;
		compressRow(w, 3, (int*)curv, yuv+2, &ic, &dc, &countId, &countDa);
		decompressRow(w, 3, (int*)curv, yuv+2, &ice, &dce);
		totId += countId;
		totDa += countDa;
		
		
		for (x=0; x<w-1;x++)
		{
			int d = abs(yuv[1+x*3] - yuv[1+(x-1)*3]);
			
			if (d>=128)			diff[0]++;
			else if (d>=64)		diff[1]++;
			else if (d>=32)		diff[2]++;
			else if (d>=16)		diff[3]++;
			else if (d>=8)		diff[4]++;
			else if (d>=4)		diff[5]++;
			else if (d>=2)		diff[6]++;
			else if (d>=1)		diff[7]++;
			else				diff[8]++;
		}
		
		for (x=1; x<w-1; x++)
		{
			unsigned char Y = yuv[x*3+y*w*3+0];
			unsigned char Pb = yuv[x*3+y*w*3+1];
			unsigned char Pr = yuv[x*3+y*w*3+2];
			
			d[0+x*bpp + y*bpp*w] = clamp(Y + ((float)Pr-128.0f)*1.13983f*0.615f/0.5f);
			d[1+x*bpp + y*bpp*w] = clamp(Y + ((float)Pb-128.0f)*(-0.39465f)*0.436f/0.5f
											+((float)Pr-128.0f)*(-0.58060f)*0.436f/0.5f);
			d[2+x*bpp + y*bpp*w] = clamp(Y + ((float)Pb-128.0f)*2.03211f*0.436f/0.5f);
		}
	}
	
	printf(" R:%0.0f\n G:%0.0f\n B:%0.0f\n Y:%0.0f\n U:%0.0f\n V:%0.0f\n RGB:%0.0f\n YUV:%0.0f\n",
			c[0], c[1], c[2], c[3], c[4], c[5],
			c[0] + c[1] + c[2], c[3] + c[4] + c[5]);
	printf(" IDs:%i\n DA:%i\n", totId, totDa);
	
	for (x=0; x<9; x++)
		printf("DIFF %i: %i\n",x, diff[x]);
	
	//For the first row, display field statistics (lossless)
	
	short *sdr = (short*)deriv;
	for (y=0; y<h; y++)
	{
		for (x=0; x<w; x++)
		{
			for (z=0; z<bpp; z++)
			{
				if (x==0)
					sdr[z + x*bpp + y*bpp*w] = s[z + x*bpp + y*bpp*w];
				else
				{
					sdr[z + x*bpp + y*bpp*w] = 
							 s[z + x*bpp + y*bpp*w]
							-s[z + (x-1)*bpp + y*bpp*w];
				}
				
				//d[z + x*bpp + y*bpp*w] = clamp(scv[z + x*bpp + y*bpp*w]&0xFF);
			}
		}
	}
	
	short *scv = (short*)curv;
	//int sbpp = bpp;
	int sbpp = 3;
	s = yuv;
	
	for (y=0; y<h; y++)
	{
		for (x=0; x<w; x++)
		{
			for (z=0; z<bpp; z++)
			{
				if (x==0)
					scv[z + x*bpp + y*bpp*w] = s[z + x*sbpp + y*sbpp*w];
				else
				{
					scv[z + x*bpp + y*bpp*w] = s[z + x*sbpp + y*sbpp*w] - s[z + (x-1)*sbpp + y*sbpp*w];
//							 s[z + x*bpp + y*bpp*w]
//							-s[z + (x-1)*bpp + y*bpp*w];
					
					if (scv[z + x*bpp + y*bpp*w] & 0x100)	//2's complemented?
					{
						scv[z + x*bpp + y*bpp*w] = -scv[z + x*bpp + y*bpp*w];
						scv[z + x*bpp + y*bpp*w] |= 0x100;
					}
				}
				
				d[z + x*bpp + y*bpp*w] = clamp(scv[z + x*bpp + y*bpp*w]&0xFF);
			}
		}
	}
	
	int b;
	unsigned short bit = 0x01;
	int byteEst = 0;
	for (b=0; b<9; b++)
	{
		printf("\n%i (%x): ", b, bit);
		
		int bin1 = 0;
		int bin2 = 0;
		int bin3 = 0;
		int bin4p = 0;
		
		int maxVal = 0;
		
		int isNumb = scv[1] & bit;
		int count = 0;
		
		int curBit = 0;
		
		for (x=0; x<w*h; x++)
		{
			if ((scv[x*bpp+1] & bit) == isNumb)
				count ++;
			else
			{
				if (isNumb)	isNumb = 1;
//				printf("%i[%i] ", isNumb, count);
				
				if (count == 1)	bin1++;
				else if (count==2)	bin2++;
				else if (count==3)	bin3++;
				else	bin4p++;
				
				if (count > maxVal) maxVal = count;
				
				curBit += 1;
				
				isNumb = scv[x*bpp+1] &bit;
				count = 1;
			}
		}
		if (isNumb)	isNumb = 1;
//		printf("%i[%i] ", isNumb, count);
		if (count > maxVal) maxVal = count;
		if (count == 1)	bin1++;
		else if (count==2)	bin2++;
		else if (count==3)	bin3++;
		else	bin4p++;
		
		curBit += 1;
		
		int numBits = 1;
		int bitNeeded = 1;
		while (maxVal > bitNeeded+1)
		{
			bitNeeded = bitNeeded << 1;
			numBits++;
		}
		
		if (curBit * numBits >= w*h)
			byteEst += w*h;
		else
			byteEst += curBit * numBits;
		
		byteEst+=4;	//Overhead...
		
		printf("(%i %i %i | %i / %i = %i %%)", bin1, bin2, bin3,
												bin1+bin2+bin3, bin4p,
												100*bin4p / (bin1+bin2+bin3 +bin4p));
		
		bit = bit << 1;
		printf("\n");
	}
	
	printf("C: %i U: %i\n", byteEst, w*h*8);
	
	//short *cX = malloc(w*h*sizeof(short));
//	short *cY = malloc(w*h*sizeof(short));
//	unsigned char *cd = malloc(w*h*bpp*sizeof(unsigned char));
//	int numPts = 0;
	
	//See if we can get more spatially distant points...
//	for (y=0; y<h; y++)
//	{
//		for (x=0; x<w; x++)
//		{
//			for (z=0; z<bpp; z++)
//			{
//				if (x==0 || y == 0 || x == w-1 || y == h-1)
//					deriv[z + x*bpp + y*bpp*w] = 0;
//				else
//				{
//					deriv[z + x*bpp + y*bpp*w] = 
//						(	 s[z + (x-1)*bpp + y*bpp*w]
//							-s[z + (x+1)*bpp + y*bpp*w]
//							+s[z + x*bpp + (y-1)*bpp*w]
//							-s[z + x*bpp + (y+1)*bpp*w])/2;
//				}
//				
//				d[z + x*bpp + y*bpp*w] = s[z + x*bpp + y*bpp*w];
//			}
//		}
//	}
	
//	for (y=0; y<h; y++)
//	{
//		for (x=0; x<w; x++)
//		{
//			for (z=0; z<bpp; z++)
//			{
//				if (x==0 || y == 0 || x == w-1 || y == h-1)
//					curv[z + x*bpp + y*bpp*w] = 0;
//				else
//				{
//					curv[z + x*bpp + y*bpp*w] = 
//						(	 deriv[z + (x-1)*bpp + y*bpp*w]
//							-deriv[z + (x+1)*bpp + y*bpp*w]
//							+deriv[z + x*bpp + (y-1)*bpp*w]
//							-deriv[z + x*bpp + (y+1)*bpp*w])/2;
//				}
//			}
//		}
//	}

//	bpp = 3;
//	s = yuv;

	/*for (y=0; y<h; y++)
	{
		for (x=0; x<w; x++)
		{
			for (z=0; z<bpp; z++)
			{
				if (x==0 || y == 0 || x == w-1 || y == h-1)
					curv[z + x*bpp + y*bpp*w] = 0;
				else
				{
					curv[z + x*bpp + y*bpp*w] = 
						(	 s[z + (x-1)*bpp + y*bpp*w]
							+s[z + (x+1)*bpp + y*bpp*w]
							+s[z + x*bpp + (y-1)*bpp*w]
							+s[z + x*bpp + (y+1)*bpp*w]
							-4*s[z + x*bpp + y*bpp*w])/4;
				}
				
				//d[z + x*bpp + y*bpp*w] = s[z + x*bpp + y*bpp*w];
			}
		}
	}
	
	//printf("%x\n", d);
	
	int skip = 0;
	for (y=0; y<h; y++)
	{
		for (x=0; x<w; x++)
		{
			float dp = 0;
			for (z=0; z<bpp; z++)
			{
				d[z + x*bpp + y*bpp*w] = clamp(abs(curv[z + x*bpp + y*bpp*w]));
				
				dp += abs(curv[z + x*bpp + y*bpp*w]);
			}
			
			if (x == 0 && (y == 0 || y == h-1))	dp = 999;
			if (y == 0 && (x == 0 || x == w-1)) dp = 999;
			
			d[0 + x*bpp + y*bpp*w] = 0;
			if (dp > 4)
			{
				//d[0 + x*bpp + y*bpp*w] = 255;
//				if (dp == 999)	skip = 0;
//				else
//				{
//					
//				}
				
				if (skip == 0)
				{
					cX[numPts] = x;
					cY[numPts] = y;
					for (z=0; z<bpp; z++)
					{
						cd[numPts*bpp + z] = s[z+x*bpp + y*bpp*w];
						//d[z + x*bpp + y*bpp*w] = 0;
					}
					
					numPts++;
				}
			}
			else
			{
				for (z=0; z<bpp; z++)
				{
					//d[z + x*bpp + y*bpp*w] = 255;
				}
			}
		}
	}
	//printf("%x\n", d);
	
	printf("%i points (out of %i) = %i%%\n", numPts, w*h, 100*numPts/(w*h));
	
#define CLOSEST_PTS		2

	float natE = 2.718281828459045f;
	//float weight1 = 1.0f;
	float weight1 = (1.0f/(sqrtf(3.1415962f))) * powf(natE, -1);
	//Now - for the work of piecing it all together... (stupid algo!)
	int minStart = 0;
	for (y=0; y<h; y++)
	{
		for (x=0; x<w; x++)
		{
			int p[CLOSEST_PTS];
			float d2[CLOSEST_PTS];
			
			int dZ = -1;
			
			for (z=0; z<CLOSEST_PTS; z++)
			{
				d2[z] = 99999;
			}
			
			//Find 'n' closest points...
			int minDist = 999999;
			for (z=minStart; z<numPts && dZ == -1 && y + 20 > cY[z]; z++)
			{
				int dd = (cX[z]-x)*(cX[z]-x) + (cY[z]-y)*(cY[z]-y);
				
				int maxDiff = 0;
				int maxBy = -1;
				
				int k;
				for (k=0; k<CLOSEST_PTS; k++)
				{
					if (d2[k] - dd > maxDiff)
					{
						maxDiff = d2[k]-dd;
						maxBy = k;
					}
				}
				
				if (maxBy >= 0)
				{
					d2[maxBy] = dd;
					p[maxBy] = z;
					
					if (dd == 0)
						dZ = maxBy;
				
					if (dd < minDist)
						minDist = dd;
				
				}
				
				if (y-20 > cY[z])	minStart = z;
				
			}
			
			if (dZ != -1)
			{
				for (z=0; z<bpp; z++)
				{
					d[z+x*bpp+y*bpp*w] = cd[z+p[dZ]*bpp];
				}
			}
			else
			{
				//Do simple Monte-Carlo to rebuild the image from key-points...
				float summed = 0;
				float maxDist = 0;
				float total[8] = {0,0,0,0,0,0,0,0};
				for (z=0; z<CLOSEST_PTS; z++)
				{
					if (d2[z] != 99999)
					{
						d2[z] = sqrtf(d2[z]);
						
						//if (d2[z] <= 0.001f)
						//	d2[z] = 10000.0f;
						//else
						//	d2[z] = 1.0f/d2[z];
						summed += (d2[z]);
					}
				}
				
				for (z=0; z<CLOSEST_PTS; z++)
				{
					if (d2[z] != 99999)
					{
						int k;
						float weight = summed - d2[z];
						//float weight = 1.0f/(d2[z]+10.0f);
						//float weight = (1.0f/(sqrtf(3.1415962f))) * powf(natE, -d2[z]*d2[z]);
						maxDist += weight;
						for (k=0; k<bpp; k++)
						{
							total[k] += (float)(cd[k+p[z]*bpp]) * weight;
							//total[k] += (float)(cd[k+p[z]*bpp]) * (20-d2[z]);
						}
					}
				}
				
//				if (x != 0)
//				{
//					
//					maxDist += weight1;
//					
//					int k;
//					for (k=0; k<bpp; k++)
//						total[k] += (float)(d[k+(x-1)*bpp+y*bpp*w])*weight1;
//				}
//				
//				if (y != 0)
//				{
//					maxDist += weight1;
//					
//					int k;
//					for (k=0; k<bpp; k++)
//						total[k] += (float)(d[k+x*bpp+(y-1)*bpp*w])*weight1;
//				}
				
				for (z=0; z<bpp; z++)
				{
					if (isinf(summed) || isinf(total[z])
						|| isnan(summed) || isinf(total[z])
						|| total[z] < 0 || summed < 0)
					{
						printf("DEBUG TIME\n");
					}
					d[z+x*bpp+y*bpp*w] = clamp(total[z]/maxDist + 0.5f);
				}
					
//				if (d[0+x*bpp+y*bpp*w] == 0
//						&& d[1+x*bpp+y*bpp*w] == 0
//						&& d[2+x*bpp+y*bpp*w] == 0)
//				{
//					printf("ZERO\n");
//				}
			}
		}
		
		printf("DONE: %f %%\n", 100.0f*(float)y/(float)h);
	}*/
	
	int minError = 0;
	int maxError = 0;
	for (y=0; y<h; y++)
	{
		for (x=0; x<w; x++)
		{
			for (z=0; z<sbpp; z++)
			{
				int error = d[z+x*bpp+y*bpp*w] - s[z+x*sbpp+y*sbpp*w];
				
				if (minError > error)
					minError = error;
				
				if (maxError < error)
					maxError = error;
				
				//d[z+x*bpp+y*bpp*w] = (error + 200)/2;
			}
			
			if (bpp > 3)
				d[x*bpp+y*bpp*w+3] = 255;
		}
	}
	
	printf("ERROR BOUNDS: %i %i\n", minError, maxError);
	
//	for (y=0; y<h; y++)
//	{
//		
//		for (x=1; x<w-1; x++)
//		{
//			unsigned char Y = d[x*3+y*w*3+0];
//			unsigned char Pb = d[x*3+y*w*3+1];
//			unsigned char Pr = d[x*3+y*w*3+2];
//			
//			d[0+x*bpp + y*bpp*w] = clamp(Y + ((float)Pr-128.0f)*1.13983f*0.615f/0.5f);
//			d[1+x*bpp + y*bpp*w] = clamp(Y + ((float)Pb-128.0f)*(-0.39465f)*0.436f/0.5f
//											+((float)Pr-128.0f)*(-0.58060f)*0.436f/0.5f);
//			d[2+x*bpp + y*bpp*w] = clamp(Y + ((float)Pb-128.0f)*2.03211f*0.436f/0.5f);
//		}
//	}
	
	[NSBundle loadNibNamed:@"FluidZip_Window" owner:self];
	[r_windows addObject:i_window];
	[i_window release];
	
	[i_window setImage:i2];
	[i_window setTitle:[[m_focus title] stringByAppendingString:@".vec"]];
	[i_window setDelegate:self];
	
	free(indices);
	free(data);
	free(curv);
	free(deriv);
	free(iData);
	free(yuv);
//	free(cX);
//	free(cY);
//	free(cd);
	
	[i2 release];
	[cpy release];
	[bir release];
}


- (void)onGotFocus:(FluidZip_Window*)in_window
{
	m_focus = in_window;
}

@end
