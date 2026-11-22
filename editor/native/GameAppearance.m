#import "GameAppearance.h"
#include <mach-o/fat.h>
#include <mach-o/loader.h>

static BOOL Fail(NSError **error, NSString *message) {
  if (error)
    *error = [NSError errorWithDomain:@"EveNGINE.Export"
                                 code:1
                             userInfo:@{NSLocalizedDescriptionKey : message}];
  return NO;
}
static int Hex(unichar c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}
static BOOL WriteIcon(NSDictionary *project, NSURL *resources, NSURL *fallback, NSError **error) {
  NSURL *output = [resources URLByAppendingPathComponent:@"GameIcon.icns"];
  id icon = project[@"appIcon"];
  if (!icon)
    return [NSFileManager.defaultManager copyItemAtURL:fallback toURL:output error:error];
  if (![icon isKindOfClass:NSDictionary.class])
    return Fail(error, @"Invalid game icon.");
  id sizeValue = icon[@"size"], pixelValue = icon[@"pixels"];
  if (![sizeValue isKindOfClass:NSNumber.class] ||
      CFGetTypeID((__bridge CFTypeRef)sizeValue) == CFBooleanGetTypeID() ||
      ![pixelValue isKindOfClass:NSString.class])
    return Fail(error, @"Invalid game icon dimensions or pixels.");
  NSInteger size = [sizeValue integerValue];
  NSString *pixels = pixelValue;
  if ([sizeValue doubleValue] != size || size < 16 || size > 512 ||
      pixels.length != (NSUInteger)(size * size * 8))
    return Fail(error, @"Game icons must be square, 16–512 pixels, with RGBA artwork.");
  NSMutableData *source = [NSMutableData dataWithLength:size * size * 4];
  unsigned char *bytes = source.mutableBytes;
  BOOL visible = NO;
  for (NSUInteger i = 0; i < source.length; i++) {
    int a = Hex([pixels characterAtIndex:i * 2]), b = Hex([pixels characterAtIndex:i * 2 + 1]);
    if (a < 0 || b < 0)
      return Fail(error, @"Invalid game icon pixel.");
    bytes[i] = (a << 4) | b;
    if (i % 4 == 3 && bytes[i])
      visible = YES;
  }
  if (!visible)
    return [NSFileManager.defaultManager copyItemAtURL:fallback toURL:output error:error];
  NSURL *iconset = [resources URLByAppendingPathComponent:@"GameIcon.iconset"];
  NSFileManager *fm = NSFileManager.defaultManager;
  if (![fm createDirectoryAtURL:iconset withIntermediateDirectories:YES attributes:nil error:error])
    return NO;
  BOOL ok = YES;
  for (NSNumber *base in @[ @16, @32, @128, @256, @512 ])
    for (NSNumber *scale in @[ @1, @2 ]) {
      NSInteger target = base.integerValue * scale.integerValue;
      NSBitmapImageRep *rep =
          [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL
                                                  pixelsWide:target
                                                  pixelsHigh:target
                                               bitsPerSample:8
                                             samplesPerPixel:4
                                                    hasAlpha:YES
                                                    isPlanar:NO
                                              colorSpaceName:NSDeviceRGBColorSpace
                                                bitmapFormat:NSBitmapFormatAlphaNonpremultiplied
                                                 bytesPerRow:target * 4
                                                bitsPerPixel:32];
      if (!rep) {
        ok = Fail(error, @"Cannot create game icon image.");
        break;
      }
      for (NSInteger y = 0; y < target; y++)
        for (NSInteger x = 0; x < target; x++)
          memcpy(rep.bitmapData + y * rep.bytesPerRow + x * 4,
                 bytes + ((y * size / target) * size + x * size / target) * 4, 4);
      NSData *png = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
      NSString *name = [NSString stringWithFormat:@"icon_%ldx%ld%@.png", (long)base.integerValue,
                                                  (long)base.integerValue,
                                                  scale.integerValue == 2 ? @"@2x" : @""];
      if (!png || ![png writeToURL:[iconset URLByAppendingPathComponent:name]
                           options:NSDataWritingAtomic
                             error:error]) {
        ok = NO;
        break;
      }
    }
  if (ok) {
    NSTask *task = [NSTask new];
    task.executableURL = [NSURL fileURLWithPath:@"/usr/bin/iconutil"];
    task.arguments = @[ @"-c", @"icns", @"-o", output.path, iconset.path ];
    NSPipe *pipe = [NSPipe pipe];
    task.standardError = pipe;
    ok = [task launchAndReturnError:error];
    if (ok) {
      NSData *diagnostic = [pipe.fileHandleForReading readDataToEndOfFile];
      [task waitUntilExit];
      if (task.terminationStatus != 0)
        ok = Fail(error, [[NSString alloc] initWithData:diagnostic encoding:NSUTF8StringEncoding]
                             ?: @"Cannot assemble game icon.");
    }
  }
  [fm removeItemAtURL:iconset error:nil];
  return ok;
}

// A string export setting from the project, or nil when it is missing or empty.
static NSString *Setting(NSDictionary *project, NSString *key) {
  id settings = project[@"export"];
  if (![settings isKindOfClass:NSDictionary.class])
    return nil;
  id value = settings[key];
  return [value isKindOfClass:NSString.class] && [value length] ? value : nil;
}
NSString *EveExportAppName(NSDictionary *project) {
  NSString *title = [project[@"title"] isKindOfClass:NSString.class] ? project[@"title"] : nil;
  NSString *name = Setting(project, @"appName") ?: (title.length ? title : @"My Game");
  // App names become file names, so slashes and colons are replaced.
  NSCharacterSet *bad = [NSCharacterSet characterSetWithCharactersInString:@"/:\\"];
  return [[name componentsSeparatedByCharactersInSet:bad] componentsJoinedByString:@"-"];
}
static NSString *BundleId(NSDictionary *project) {
  NSString *custom = Setting(project, @"bundleId");
  if (custom)
    return custom;
  NSString *projectId = [project[@"id"] isKindOfClass:NSString.class] ? project[@"id"] : @"game";
  NSMutableString *clean = [NSMutableString new];
  for (NSUInteger i = 0; i < projectId.length; i++) {
    unichar c = [projectId characterAtIndex:i];
    BOOL ok =
        (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-';
    [clean appendFormat:@"%C", ok ? c : (unichar)'-'];
  }
  return [@"dev.lota.game." stringByAppendingString:clean];
}
// Keeps one architecture of a universal Mach-O file. Does nothing when the file already has
// only that architecture. Reads the fat header itself, so players' Macs need no developer tools.
static BOOL Thin(NSURL *file, cpu_type_t cpu, NSString *arch, NSError **error) {
  NSData *data = [NSData dataWithContentsOfURL:file options:0 error:error];
  if (!data)
    return NO;
  const uint8_t *bytes = data.bytes;
  if (data.length < 8)
    return Fail(error, [file.lastPathComponent stringByAppendingString:@" is not a program."]);
  uint32_t magic = OSSwapBigToHostInt32(*(const uint32_t *)bytes);
  if (magic != FAT_MAGIC && magic != FAT_MAGIC_64) {
    const struct mach_header_64 *header = (const struct mach_header_64 *)bytes;
    if (header->magic == MH_MAGIC_64 && header->cputype == cpu)
      return YES;
    return Fail(error,
                [NSString stringWithFormat:@"%@ has no %@ version.", file.lastPathComponent, arch]);
  }
  uint32_t count = OSSwapBigToHostInt32(((const struct fat_header *)bytes)->nfat_arch);
  BOOL wide = magic == FAT_MAGIC_64;
  size_t entry = wide ? sizeof(struct fat_arch_64) : sizeof(struct fat_arch);
  if (count > 16 || data.length < sizeof(struct fat_header) + count * entry)
    return Fail(error, [file.lastPathComponent stringByAppendingString:@" is damaged."]);
  for (uint32_t i = 0; i < count; i++) {
    const uint8_t *at = bytes + sizeof(struct fat_header) + i * entry;
    cpu_type_t type;
    uint64_t offset, size;
    if (wide) {
      const struct fat_arch_64 *a = (const struct fat_arch_64 *)at;
      type = (cpu_type_t)OSSwapBigToHostInt32((uint32_t)a->cputype);
      offset = OSSwapBigToHostInt64(a->offset);
      size = OSSwapBigToHostInt64(a->size);
    } else {
      const struct fat_arch *a = (const struct fat_arch *)at;
      type = (cpu_type_t)OSSwapBigToHostInt32((uint32_t)a->cputype);
      offset = OSSwapBigToHostInt32(a->offset);
      size = OSSwapBigToHostInt32(a->size);
    }
    if (type != cpu)
      continue;
    if (offset > data.length || size > data.length - offset)
      return Fail(error, [file.lastPathComponent stringByAppendingString:@" is damaged."]);
    NSNumber *permissions =
        [NSFileManager.defaultManager attributesOfItemAtPath:file.path
                                                       error:nil][NSFilePosixPermissions];
    if (![[data subdataWithRange:NSMakeRange(offset, size)] writeToURL:file
                                                               options:NSDataWritingAtomic
                                                                 error:error])
      return NO;
    if (permissions)
      [NSFileManager.defaultManager setAttributes:@{NSFilePosixPermissions : permissions}
                                     ofItemAtPath:file.path
                                            error:nil];
    return YES;
  }
  return Fail(error,
              [NSString stringWithFormat:@"%@ has no %@ version.", file.lastPathComponent, arch]);
}
static BOOL Run(NSString *tool, NSArray<NSString *> *arguments, NSError **error) {
  NSTask *task = [NSTask new];
  task.executableURL = [NSURL fileURLWithPath:tool];
  task.arguments = arguments;
  NSPipe *pipe = [NSPipe pipe];
  task.standardError = pipe;
  task.standardOutput = NSFileHandle.fileHandleWithNullDevice;
  if (![task launchAndReturnError:error])
    return NO;
  NSData *diagnostic = [pipe.fileHandleForReading readDataToEndOfFile];
  [task waitUntilExit];
  if (task.terminationStatus == 0)
    return YES;
  NSString *text = [[NSString alloc] initWithData:diagnostic encoding:NSUTF8StringEncoding];
  text = [text stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
  return Fail(error,
              text.length ? text : [tool.lastPathComponent stringByAppendingString:@" failed."]);
}
// Moves a finished file into place, replacing what is there.
static BOOL Place(NSURL *stage, NSURL *destination, NSError **error) {
  NSFileManager *fm = NSFileManager.defaultManager;
  if ([fm fileExistsAtPath:destination.path]) {
    NSURL *result = nil;
    return [fm replaceItemAtURL:destination
                  withItemAtURL:stage
                 backupItemName:nil
                        options:0
               resultingItemURL:&result
                          error:error];
  }
  return [fm moveItemAtURL:stage toURL:destination error:error];
}

BOOL EveExportGame(NSDictionary *project, NSURL *runtime, NSURL *font, NSURL *framework,
                   NSURL *defaultIcon, NSURL *destination, NSError **error) {
  NSFileManager *fm = NSFileManager.defaultManager;
  NSURL *stage = [[destination URLByDeletingLastPathComponent]
      URLByAppendingPathComponent:[@".eve-export-" stringByAppendingString:NSUUID.UUID.UUIDString]];
  NSURL *contents = [stage URLByAppendingPathComponent:@"Contents"],
        *resources = [contents URLByAppendingPathComponent:@"Resources"];
  BOOL ok = YES;
  for (NSString *folder in @[ @"MacOS", @"Resources", @"Frameworks" ])
    if (![fm createDirectoryAtURL:[contents URLByAppendingPathComponent:folder]
            withIntermediateDirectories:YES
                             attributes:nil
                                  error:error]) {
      ok = NO;
      break;
    }
  if (ok)
    ok = [fm copyItemAtURL:runtime
                     toURL:[contents URLByAppendingPathComponent:@"MacOS/Game"]
                     error:error];
  NSData *data = ok ? [NSJSONSerialization dataWithJSONObject:project
                                                      options:NSJSONWritingPrettyPrinted
                                                        error:error]
                    : nil;
  if (ok)
    ok = data && [data writeToURL:[resources URLByAppendingPathComponent:@"project.lota"]
                          options:NSDataWritingAtomic
                            error:error];
  if (ok)
    ok = [fm copyItemAtURL:font
                     toURL:[resources URLByAppendingPathComponent:@"font.pam"]
                     error:error];
  if (ok)
    ok = [fm copyItemAtURL:framework
                     toURL:[contents URLByAppendingPathComponent:@"Frameworks/SDL3.framework"]
                     error:error];
  if (ok)
    ok = WriteIcon(project, resources, defaultIcon, error);
  NSString *architecture = Setting(project, @"macArchitecture") ?: @"universal";
  if (ok && ![architecture isEqual:@"universal"]) {
    BOOL arm = [architecture isEqual:@"arm64"];
    if (!arm && ![architecture isEqual:@"x86_64"])
      ok = Fail(error, @"Unknown Mac architecture.");
    NSString *label = arm ? @"Apple Silicon" : @"Intel";
    cpu_type_t cpu = arm ? CPU_TYPE_ARM64 : CPU_TYPE_X86_64;
    if (ok)
      ok = Thin([contents URLByAppendingPathComponent:@"MacOS/Game"], cpu, label, error);
    if (ok)
      ok = Thin([contents URLByAppendingPathComponent:@"Frameworks/SDL3.framework/Versions/A/SDL3"],
                cpu, label, error);
  }
  NSString *name = EveExportAppName(project), *version = Setting(project, @"version") ?: @"1.0.0";
  NSDictionary *plist = @{
    @"CFBundleName" : name,
    @"CFBundleDisplayName" : name,
    @"CFBundleExecutable" : @"Game",
    @"CFBundleIdentifier" : BundleId(project),
    @"CFBundlePackageType" : @"APPL",
    @"CFBundleVersion" : version,
    @"CFBundleShortVersionString" : version,
    @"CFBundleIconFile" : @"GameIcon",
    @"LSMinimumSystemVersion" : @"12.0",
    @"NSHighResolutionCapable" : @YES,
    @"SDL_FILESYSTEM_BASE_DIR_TYPE" : @"resource"
  };
  NSData *info = ok ? [NSPropertyListSerialization dataWithPropertyList:plist
                                                                 format:NSPropertyListXMLFormat_v1_0
                                                                options:0
                                                                  error:error]
                    : nil;
  if (ok)
    ok = info && [info writeToURL:[contents URLByAppendingPathComponent:@"Info.plist"]
                          options:NSDataWritingAtomic
                            error:error];
  if (ok)
    ok = [[@"APPL????" dataUsingEncoding:NSASCIIStringEncoding]
        writeToURL:[contents URLByAppendingPathComponent:@"PkgInfo"]
           options:NSDataWritingAtomic
             error:error];
  if (ok)
    ok = Place(stage, destination, error);
  if (!ok)
    [fm removeItemAtURL:stage error:nil];
  return ok;
}

BOOL EvePackageGame(NSURL *app, NSString *format, NSString *name, NSURL *destination,
                    NSError **error) {
  NSFileManager *fm = NSFileManager.defaultManager;
  NSURL *work = [[destination URLByDeletingLastPathComponent]
      URLByAppendingPathComponent:[@".eve-package-"
                                      stringByAppendingString:NSUUID.UUID.UUIDString]];
  NSURL *output = [work URLByAppendingPathComponent:[@"package." stringByAppendingString:format]];
  BOOL ok = [fm createDirectoryAtURL:work
         withIntermediateDirectories:YES
                          attributes:nil
                               error:error];
  if (ok && [format isEqual:@"zip"])
    // ditto keeps the app's symlinks, permissions and signatures, which zip(1) can lose.
    ok = Run(@"/usr/bin/ditto",
             @[ @"-c", @"-k", @"--sequesterRsrc", @"--keepParent", app.path, output.path ], error);
  else if (ok && [format isEqual:@"dmg"]) {
    NSURL *folder = [work URLByAppendingPathComponent:@"disk"];
    ok = [fm createDirectoryAtURL:folder
             withIntermediateDirectories:YES
                              attributes:nil
                                   error:error] &&
         [fm copyItemAtURL:app
                     toURL:[folder URLByAppendingPathComponent:app.lastPathComponent]
                     error:error] &&
         // A shortcut to Applications lets players drag the game across to install it.
         [fm createSymbolicLinkAtURL:[folder URLByAppendingPathComponent:@"Applications"]
                  withDestinationURL:[NSURL fileURLWithPath:@"/Applications"]
                               error:error] &&
         Run(@"/usr/bin/hdiutil",
             @[
               @"create", @"-volname", name, @"-srcfolder", folder.path, @"-fs", @"HFS+",
               @"-format", @"UDZO", @"-ov", output.path
             ],
             error);
  } else if (ok)
    ok = Fail(error, [@"Unknown package format " stringByAppendingString:format]);
  if (ok)
    ok = Place(output, destination, error);
  [fm removeItemAtURL:work error:nil];
  return ok;
}
