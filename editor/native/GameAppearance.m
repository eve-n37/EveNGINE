#import "GameAppearance.h"

static BOOL Fail(NSError **error, NSString *message) {
 if(error)*error=[NSError errorWithDomain:@"EveNGINE.Export" code:1 userInfo:@{NSLocalizedDescriptionKey:message}];
 return NO;
}
static int Hex(unichar c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}
static BOOL WriteIcon(NSDictionary *project,NSURL *resources,NSURL *fallback,NSError **error){
 NSURL *output=[resources URLByAppendingPathComponent:@"GameIcon.icns"];
 id icon=project[@"appIcon"];
 if(!icon)return [NSFileManager.defaultManager copyItemAtURL:fallback toURL:output error:error];
 if(![icon isKindOfClass:NSDictionary.class])return Fail(error,@"Invalid game icon.");
 id sizeValue=icon[@"size"],pixelValue=icon[@"pixels"];
 if(![sizeValue isKindOfClass:NSNumber.class]||CFGetTypeID((__bridge CFTypeRef)sizeValue)==CFBooleanGetTypeID()||![pixelValue isKindOfClass:NSString.class])return Fail(error,@"Invalid game icon dimensions or pixels.");
 NSInteger size=[sizeValue integerValue];NSString *pixels=pixelValue;
 if([sizeValue doubleValue]!=size||size<16||size>512||pixels.length!=(NSUInteger)(size*size*8))return Fail(error,@"Game icons must be square, 16–512 pixels, with RGBA artwork.");
 NSMutableData *source=[NSMutableData dataWithLength:size*size*4];unsigned char *bytes=source.mutableBytes;BOOL visible=NO;
 for(NSUInteger i=0;i<source.length;i++){int a=Hex([pixels characterAtIndex:i*2]),b=Hex([pixels characterAtIndex:i*2+1]);if(a<0||b<0)return Fail(error,@"Invalid game icon pixel.");bytes[i]=(a<<4)|b;if(i%4==3&&bytes[i])visible=YES;}
 if(!visible)return [NSFileManager.defaultManager copyItemAtURL:fallback toURL:output error:error];
 NSURL *iconset=[resources URLByAppendingPathComponent:@"GameIcon.iconset"];
 NSFileManager *fm=NSFileManager.defaultManager;if(![fm createDirectoryAtURL:iconset withIntermediateDirectories:YES attributes:nil error:error])return NO;
 BOOL ok=YES;
 for(NSNumber *base in @[@16,@32,@128,@256,@512])for(NSNumber *scale in @[@1,@2]){
  NSInteger target=base.integerValue*scale.integerValue;
  NSBitmapImageRep *rep=[[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:target pixelsHigh:target bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace bitmapFormat:NSBitmapFormatAlphaNonpremultiplied bytesPerRow:target*4 bitsPerPixel:32];
  if(!rep){ok=Fail(error,@"Cannot create game icon image.");break;}
  for(NSInteger y=0;y<target;y++)for(NSInteger x=0;x<target;x++)memcpy(rep.bitmapData+y*rep.bytesPerRow+x*4,bytes+((y*size/target)*size+x*size/target)*4,4);
  NSData *png=[rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];NSString *name=[NSString stringWithFormat:@"icon_%ldx%ld%@.png",(long)base.integerValue,(long)base.integerValue,scale.integerValue==2?@"@2x":@""];
  if(!png||![png writeToURL:[iconset URLByAppendingPathComponent:name] options:NSDataWritingAtomic error:error]){ok=NO;break;}
 }
 if(ok){NSTask *task=[NSTask new];task.executableURL=[NSURL fileURLWithPath:@"/usr/bin/iconutil"];task.arguments=@[@"-c",@"icns",@"-o",output.path,iconset.path];NSPipe *pipe=[NSPipe pipe];task.standardError=pipe;
  ok=[task launchAndReturnError:error];if(ok){NSData *diagnostic=[pipe.fileHandleForReading readDataToEndOfFile];[task waitUntilExit];if(task.terminationStatus!=0)ok=Fail(error,[[NSString alloc] initWithData:diagnostic encoding:NSUTF8StringEncoding]?:@"Cannot assemble game icon.");}
 }
 [fm removeItemAtURL:iconset error:nil];return ok;
}

BOOL EveExportGame(NSDictionary *project,NSURL *runtime,NSURL *font,NSURL *framework,NSURL *defaultIcon,NSURL *destination,NSError **error){
 NSFileManager *fm=NSFileManager.defaultManager;NSURL *stage=[[destination URLByDeletingLastPathComponent] URLByAppendingPathComponent:[@".eve-export-" stringByAppendingString:NSUUID.UUID.UUIDString]];
 NSURL *contents=[stage URLByAppendingPathComponent:@"Contents"],*resources=[contents URLByAppendingPathComponent:@"Resources"];
 BOOL ok=YES;for(NSString *folder in @[@"MacOS",@"Resources",@"Frameworks"])if(![fm createDirectoryAtURL:[contents URLByAppendingPathComponent:folder] withIntermediateDirectories:YES attributes:nil error:error]){ok=NO;break;}
 if(ok)ok=[fm copyItemAtURL:runtime toURL:[contents URLByAppendingPathComponent:@"MacOS/Game"] error:error];
 NSData *data=ok?[NSJSONSerialization dataWithJSONObject:project options:NSJSONWritingPrettyPrinted error:error]:nil;
 if(ok)ok=data&&[data writeToURL:[resources URLByAppendingPathComponent:@"project.lota"] options:NSDataWritingAtomic error:error];
 if(ok)ok=[fm copyItemAtURL:font toURL:[resources URLByAppendingPathComponent:@"font.pam"] error:error];
 if(ok)ok=[fm copyItemAtURL:framework toURL:[contents URLByAppendingPathComponent:@"Frameworks/SDL3.framework"] error:error];
 if(ok)ok=WriteIcon(project,resources,defaultIcon,error);
 NSString *title=project[@"title"]?:@"My Game";
 NSDictionary *plist=@{@"CFBundleName":title,@"CFBundleDisplayName":title,@"CFBundleExecutable":@"Game",@"CFBundleIdentifier":[@"dev.lota.game." stringByAppendingString:project[@"id"]],@"CFBundlePackageType":@"APPL",@"CFBundleVersion":@"1.0.0",@"CFBundleShortVersionString":@"1.0.0",@"CFBundleIconFile":@"GameIcon",@"LSMinimumSystemVersion":@"12.0",@"NSHighResolutionCapable":@YES,@"SDL_FILESYSTEM_BASE_DIR_TYPE":@"resource"};
 NSData *info=ok?[NSPropertyListSerialization dataWithPropertyList:plist format:NSPropertyListXMLFormat_v1_0 options:0 error:error]:nil;
 if(ok)ok=info&&[info writeToURL:[contents URLByAppendingPathComponent:@"Info.plist"] options:NSDataWritingAtomic error:error];
 if(ok)ok=[[@"APPL????" dataUsingEncoding:NSASCIIStringEncoding] writeToURL:[contents URLByAppendingPathComponent:@"PkgInfo"] options:NSDataWritingAtomic error:error];
 if(ok){if([fm fileExistsAtPath:destination.path]){NSURL *result=nil;ok=[fm replaceItemAtURL:destination withItemAtURL:stage backupItemName:nil options:0 resultingItemURL:&result error:error];}else ok=[fm moveItemAtURL:stage toURL:destination error:error];}
 if(!ok)[fm removeItemAtURL:stage error:nil];return ok;
}
