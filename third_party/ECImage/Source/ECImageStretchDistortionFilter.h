#import "ECImageFilter.h"

/** Creates a stretch distortion of the image
 */
@interface ECImageStretchDistortionFilter : ECImageFilter {
    GLint centerUniform;
}

/** The center about which to apply the distortion, with a default of (0.5, 0.5)
 */
@property(readwrite, nonatomic) CGPoint center;

@end
