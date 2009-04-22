//
//  X_NumberFormatter.m
//  FluidApp
//

#import "X_NumberFormatter.h"


@implementation X_NumberFormatter

- (BOOL)isPartialStringValid:(NSString *)partialString newEditingString:(NSString **)newString errorDescription:(NSString **)error
{
	int x;
	int l = [partialString length];
	
	BOOL foundDecimal = NO;
	
	int curPostDecimal = 0;
	
	for (x=0; x<l; x++)
	{
		unichar curChar = [partialString characterAtIndex:x];
		
		if (!(curChar >= '0' && curChar <= '9') && curChar != '.' &&
			!(x ==0 && (curChar == '+' || curChar == '-')))
			return NO;
		
		if (curChar >= '0' && curChar <= '9')
		{
			curPostDecimal++;
			if (curPostDecimal > 12)
				return NO;
		}
		
		if (curChar == '.')
		{
			if (foundDecimal)
				return NO;
			
			foundDecimal = YES;
		}
	}
	
	return YES;
}

- (BOOL)getObjectValue:(id *)anObject forString:(NSString *)string errorDescription:(NSString **)error
{
	*anObject = [NSNumber numberWithDouble:[string doubleValue]];
	
	return YES;
}

- (NSString *)stringForObjectValue:(id)anObject
{
	if (![anObject isKindOfClass:[NSNumber class]])
		return nil;
	
	return [NSString stringWithFormat:@"%1.12g", [anObject doubleValue]];
}

@end
