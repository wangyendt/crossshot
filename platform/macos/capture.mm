#include "capture.hpp"
#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#include <atomic>
namespace crossshot {
bool screenPermission() {
    return CGPreflightScreenCaptureAccess();
}
bool requestScreenPermission() {
    return CGRequestScreenCaptureAccess();
}
static QImage imageFromCG(CGImageRef cg) {
    if (!cg)
        return {};
    size_t w = CGImageGetWidth(cg), h = CGImageGetHeight(cg);
    if (w == 0 || h == 0 || w * h > 100000000)
        return {};
    QImage image(static_cast<int>(w), static_cast<int>(h), QImage::Format_ARGB32_Premultiplied);
    auto cs = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    auto ctx = CGBitmapContextCreate(image.bits(), w, h, 8, image.bytesPerLine(), cs,
                                     static_cast<CGBitmapInfo>(kCGImageAlphaPremultipliedFirst) |
                                         kCGBitmapByteOrder32Little);
    CGColorSpaceRelease(cs);
    if (!ctx)
        return {};
    CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), cg);
    CGContextRelease(ctx);
    return image;
}
void capturePrimary(std::function<void(CaptureResult)> callback) {
    if (!screenPermission()) {
        callback({{}, "screen_permission_required", 0});
        return;
    }
    auto timer = std::make_shared<QElapsedTimer>();
    timer->start();
    auto completed = std::make_shared<std::atomic_bool>(false);
    auto finish = [callback, timer, completed](QImage image, QString error) {
        double ms = timer->nsecsElapsed() / 1e6;
        dispatch_async(dispatch_get_main_queue(), ^{
          if (!completed->exchange(true))
              callback({image, error, ms});
        });
    };
    QTimer::singleShot(10000, [finish] { finish({}, "capture_timeout"); });
    [SCShareableContent
        getShareableContentExcludingDesktopWindows:NO
                               onScreenWindowsOnly:YES
                                 completionHandler:^(SCShareableContent *content, NSError *error) {
                                   if (error) {
                                       finish({},
                                              QString::fromNSString(error.localizedDescription));
                                       return;
                                   }
                                   SCDisplay *display = nil;
                                   for (SCDisplay *d in content.displays)
                                       if (d.displayID == CGMainDisplayID()) {
                                           display = d;
                                           break;
                                       }
                                   if (!display) {
                                       finish({}, "primary_display_not_found");
                                       return;
                                   }
                                   SCContentFilter *filter =
                                       [[SCContentFilter alloc] initWithDisplay:display
                                                               excludingWindows:@[]];
                                   SCStreamConfiguration *config = [SCStreamConfiguration new];
                                   auto mode = CGDisplayCopyDisplayMode(display.displayID);
                                   config.width = mode ? CGDisplayModeGetPixelWidth(mode)
                                                       : CGDisplayPixelsWide(display.displayID);
                                   config.height = mode ? CGDisplayModeGetPixelHeight(mode)
                                                        : CGDisplayPixelsHigh(display.displayID);
                                   if (mode)
                                       CGDisplayModeRelease(mode);
                                   config.showsCursor = NO;
                                   config.pixelFormat = kCVPixelFormatType_32BGRA;
                                   config.colorSpaceName = kCGColorSpaceSRGB;
                                   [SCScreenshotManager
                                       captureImageWithFilter:filter
                                                configuration:config
                                            completionHandler:^(CGImageRef image, NSError *err) {
                                              if (err) {
                                                  finish({}, QString::fromNSString(
                                                                 err.localizedDescription));
                                                  return;
                                              }
                                              auto converted = imageFromCG(image);
                                              finish(converted, converted.isNull() ? "empty_capture"
                                                                                   : QString());
                                            }];
                                 }];
}
void configureNativePin(QWidget *w) {
    NSView *view = (__bridge NSView *)reinterpret_cast<void *>(w->winId());
    NSWindow *win = view.window;
    if (!win)
        return;
    win.level = NSFloatingWindowLevel;
    win.collectionBehavior =
        NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorFullScreenAuxiliary;
}
QJsonObject nativePinInfo(QWidget *w) {
    NSView *view = (__bridge NSView *)reinterpret_cast<void *>(w->winId());
    NSWindow *win = view.window;
    return {{"native_window", win != nil},
            {"level", static_cast<int>(win.level)},
            {"floating", win.level >= NSFloatingWindowLevel},
            {"alpha", win.alphaValue},
            {"visible", static_cast<bool>(win.visible)}};
}
} // namespace crossshot
