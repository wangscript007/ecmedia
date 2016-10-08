//
//  video_capture_recursive_lock.mm
//
//
#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_12
#import "video_capture_recursive_lock.h"

@implementation VideoCaptureRecursiveLock

@synthesize locked = _locked;

- (id)init{
    self = [super init];
    if(nil == self){
        return nil;
    }

    [self setLocked:NO];
    return self;
}

- (void)lock{
    [self setLocked:YES];
    [super lock];
}

- (void)unlock{
    [self setLocked:NO];
    [super unlock];
}


@end
#endif
