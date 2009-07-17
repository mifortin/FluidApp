//
//  FluidAppDocument.m
//  FluidApp
//

#import "FluidAppDocument.h"


@implementation FluidAppDocument

- (NSString *)windowNibName
{
    return @"FluidAppDocument";
}

- (NSData *)dataRepresentationOfType:(NSString *)type {
    // Implement to provide a persistent data representation of your document OR remove this and implement the file-wrapper or file path based save methods.
    return nil;
}

- (BOOL)loadDataRepresentation:(NSData *)data ofType:(NSString *)type {
    // Implement to load a persistent data representation of your document OR remove this and implement the file-wrapper or file path based load methods.
    return YES;
}

@end
