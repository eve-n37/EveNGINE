#import <Cocoa/Cocoa.h>

BOOL EveExportGame(NSDictionary *project, NSURL *runtime, NSURL *font, NSURL *framework,
                   NSURL *defaultIcon, NSURL *destination, NSError **error);
// The exported app's name: the export setting, else the game title, safe to use as a file name.
NSString *EveExportAppName(NSDictionary *project);
// Packs an exported app into a "zip" archive or a "dmg" disk image at destination.
BOOL EvePackageGame(NSURL *app, NSString *format, NSString *name, NSURL *destination,
                    NSError **error);
