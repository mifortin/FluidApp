//
//  FluidFPSView.h
//  FluidApp
//

#import <Cocoa/Cocoa.h>

#define MAXFPSVIEWS		16
#define MAXFPSSAMPLES	128

@interface FluidFPSView : NSView
{
	NSString *m_titles[MAXFPSVIEWS];
	NSColor *m_colors[MAXFPSVIEWS];
	float m_samples[MAXFPSVIEWS][MAXFPSSAMPLES];
	NSRect m_regions[MAXFPSVIEWS];
	BOOL m_enabled[MAXFPSVIEWS];
	int m_curSample;
}

- (void)setTitle:(NSString*)in_title forIndex:(int)in_index;
- (void)setColor:(NSColor*)in_title forIndex:(int)in_index;
- (void)setSample:(float)in_sample forIndex:(int)in_index;

- (void)tick;
- (BOOL)visible;
@end
