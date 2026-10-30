#include "NativeTitleBranding.h"
#include "MyaBrandAssets.h"
#include "MyaIdentity.h"
#import <Cocoa/Cocoa.h>

@interface MyaNativeTitleBrandView : NSView
@end
@implementation MyaNativeTitleBrandView
- (NSView*)hitTest:(NSPoint)point { (void)point; return nil; }
- (NSInteger)tag { return 0x4d594142; }
@end

static NSString* myaNativeTitle(){return [NSString stringWithUTF8String:MyaIdentity::officialUtf8];}
static NSFont* myaNativeTitleFont(){return [NSFont systemFontOfSize:14 weight:NSFontWeightSemibold];}
double myaNativeTitleRequiredWidth(){return 24+ceil([myaNativeTitle() sizeWithAttributes:@{NSFontAttributeName:myaNativeTitleFont()}].width)+4;}

void installMyaNativeTitleBrand(juce::Component& component){
 auto* peer=component.getPeer();if(peer==nullptr)return;
 NSView* host=(NSView*)peer->getNativeHandle();NSWindow* window=[host window];if(window==nil)return;
 NSView* close=[window standardWindowButton:NSWindowCloseButton];NSView* bar=[close superview];if(bar==nil)return;while([bar superview]!=nil&&NSWidth([bar bounds])<NSWidth([window frame])*.8)bar=[bar superview];
 constexpr NSInteger tag=0x4d594142;[[bar viewWithTag:tag] removeFromSuperview];
 [window setTitle: myaNativeTitle()];[window setTitleVisibility:NSWindowTitleHidden];
 NSFont* font=myaNativeTitleFont();CGFloat width=(CGFloat)myaNativeTitleRequiredWidth(),textWidth=width-24,height=22;NSRect bounds=[bar bounds];NSRect control=[bar convertRect:[close bounds] fromView:close];
 MyaNativeTitleBrandView* brand=[[MyaNativeTitleBrandView alloc] initWithFrame:NSMakeRect((NSWidth(bounds)-width)*.5,NSMidY(control)-height*.5,width,height)];
 [brand setAutoresizingMask:NSViewMinXMargin|NSViewMaxXMargin|([bar isFlipped]?NSViewMaxYMargin:NSViewMinYMargin)];
 NSData* data=[NSData dataWithBytes:MyaBrandAssets::myaanalyzericonv1_png length:(NSUInteger)MyaBrandAssets::myaanalyzericonv1_pngSize];NSImage* image=[[NSImage alloc] initWithData:data];
 NSImageView* icon=[[NSImageView alloc] initWithFrame:NSMakeRect(0,2,18,18)];[icon setImage:image];[icon setImageScaling:NSImageScaleProportionallyUpOrDown];[brand addSubview:icon];
 NSTextField* text=[[NSTextField alloc] initWithFrame:NSMakeRect(24,1,textWidth,20)];[text setStringValue:myaNativeTitle()];[text setFont:font];[text setTextColor:[NSColor labelColor]];[text setEditable:NO];[text setSelectable:NO];[text setBezeled:NO];[text setDrawsBackground:NO];[text setAlignment:NSTextAlignmentLeft];[brand addSubview:text];
 [bar addSubview:brand];[text release];[icon release];[image release];[brand release];
}
