// AFURLResponseSerialization.m
// Copyright (c) 2011–2016 Alamofire Software Foundation ( http://alamofire.org/ )
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#import "AFURLResponseSerialization.h"

#import <TargetConditionals.h>

#if TARGET_OS_IOS
#import <UIKit/UIKit.h>
#elif TARGET_OS_WATCH
#import <WatchKit/WatchKit.h>
#elif defined(__MAC_OS_X_VERSION_MIN_REQUIRED)
#import <Cocoa/Cocoa.h>
#endif

NSString * const AFURLResponseSerializationErrorDomain = @"com.alamofire.error.serialization.response";
NSString * const AFNetworkingOperationFailingURLResponseErrorKey = @"com.alamofire.serialization.response.error.response";
NSString * const AFNetworkingOperationFailingURLResponseDataErrorKey = @"com.alamofire.serialization.response.error.data";

static NSError * AFErrorWithUnderlyingError(NSError *error, NSError *underlyingError) {
    if (!error) {
        return underlyingError;
    }

    if (!underlyingError || error.userInfo[NSUnderlyingErrorKey]) {
        return error;
    }

    NSMutableDictionary *mutableUserInfo = [error.userInfo mutableCopy];
    mutableUserInfo[NSUnderlyingErrorKey] = underlyingError;

    return [[NSError alloc] initWithDomain:error.domain code:error.code userInfo:mutableUserInfo];
}

// // 检测错误或者有限错误中是否匹配code和domain
static BOOL AFErrorOrUnderlyingErrorHasCodeInDomain(NSError *error, NSInteger code, NSString *domain) {
    // error中的domain和code相同，直接返回YES
    if ([error.domain isEqualToString:domain] && error.code == code) {
        return YES;
    } else if (error.userInfo[NSUnderlyingErrorKey]) { // 否则检测error中的NSUnderlyingErrorKey
        return AFErrorOrUnderlyingErrorHasCodeInDomain(error.userInfo[NSUnderlyingErrorKey], code, domain);
    }

    return NO;
}

// 该方法用于删除一个对象中的NUll
id AFJSONObjectByRemovingKeysWithNullValues(id JSONObject, NSJSONReadingOptions readingOptions) {
    // 1.数组
    if ([JSONObject isKindOfClass:[NSArray class]]) {
        //1.1 创建一个可变的数组，为了提高性能，使用Capacity创建
        NSMutableArray *mutableArray = [NSMutableArray arrayWithCapacity:[(NSArray *)JSONObject count]];
        // 1.2 遍历数组，通过迭代的手段清空数组内的null
        for (id value in (NSArray *)JSONObject) {
            if (![value isEqual:[NSNull null]]) {
                [mutableArray addObject:AFJSONObjectByRemovingKeysWithNullValues(value, readingOptions)];
            }
        }
        // 1.3 readingOptions == NSJSONReadingMutableContainers 返回可变容器，其他返回不可变容器
        return (readingOptions & NSJSONReadingMutableContainers) ? mutableArray : [NSArray arrayWithArray:mutableArray];
    } else if ([JSONObject isKindOfClass:[NSDictionary class]]) { // 2. 思想同上
        NSMutableDictionary *mutableDictionary = [NSMutableDictionary dictionaryWithDictionary:JSONObject];
        for (id <NSCopying> key in [(NSDictionary *)JSONObject allKeys]) {
            id value = (NSDictionary *)JSONObject[key];
            if (!value || [value isEqual:[NSNull null]]) {
                [mutableDictionary removeObjectForKey:key];
            } else if ([value isKindOfClass:[NSArray class]] || [value isKindOfClass:[NSDictionary class]]) {
                mutableDictionary[key] = AFJSONObjectByRemovingKeysWithNullValues(value, readingOptions);
            }
        }

        return (readingOptions & NSJSONReadingMutableContainers) ? mutableDictionary : [NSDictionary dictionaryWithDictionary:mutableDictionary];
    }

    return JSONObject;
}

@implementation AFHTTPResponseSerializer

+ (instancetype)serializer {
    return [[self alloc] init];
}

- (instancetype)init {
    self = [super init];
    if (!self) {
        return nil;
    }

    self.acceptableStatusCodes = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(200, 100)]; // 默认接受的状态码范围为200 ~ 299
    self.acceptableContentTypes = nil;

    return self;
}
/** NSIndexSet是一个有序的，唯一的，无符号整数的集合
 * 1. 它是一个无符号整数的集合
 * 2. 使用addIndex方法可以添加单个整数值，使用addIndexesInRange可以添加一个范围，比如NSMakeRange(20, 10) 取20包括20后边十个整数
 * 3. 唯一性，集合内的整数不会重复出现
 */

#pragma mark -

- (BOOL)validateResponse:(NSHTTPURLResponse *)response
                    data:(NSData *)data
                   error:(NSError * __autoreleasing *)error
{
    // 1. 默认responseIsValid == yes
    BOOL responseIsValid = YES;
    NSError *validationError = nil;
    // 2. 假如response存在且类型是NSHTTPURLResponse
    if ([response isKindOfClass:[NSHTTPURLResponse class]]) {
        // 2.1 条件：self.acceptableContentTypes存在且不包含服务器返回的MIMEType且MIMEType和data都不能为空
        if (self.acceptableContentTypes && ![self.acceptableContentTypes containsObject:[response MIMEType]] &&
            !([response MIMEType] == nil && [data length] == 0)) {

            if ([data length] > 0 && [response URL]) {
                // 2.1.1 生成错误信息
                NSMutableDictionary *mutableUserInfo = [@{
                                                          NSLocalizedDescriptionKey: [NSString stringWithFormat:NSLocalizedStringFromTable(@"Request failed: unacceptable content-type: %@", @"AFNetworking", nil), [response MIMEType]],
                                                          NSURLErrorFailingURLErrorKey:[response URL],
                                                          AFNetworkingOperationFailingURLResponseErrorKey: response,
                                                        } mutableCopy];
                // 2.1.2 包含data错误信息
                if (data) {
                    mutableUserInfo[AFNetworkingOperationFailingURLResponseDataErrorKey] = data;
                }
                // 2.1.3 生成NSError
                validationError = AFErrorWithUnderlyingError([NSError errorWithDomain:AFURLResponseSerializationErrorDomain code:NSURLErrorCannotDecodeContentData userInfo:mutableUserInfo], validationError);
            }

            responseIsValid = NO;
        }
        // 同上
        if (self.acceptableStatusCodes && ![self.acceptableStatusCodes containsIndex:(NSUInteger)response.statusCode] && [response URL]) {
            NSMutableDictionary *mutableUserInfo = [@{
                                               NSLocalizedDescriptionKey: [NSString stringWithFormat:NSLocalizedStringFromTable(@"Request failed: %@ (%ld)", @"AFNetworking", nil), [NSHTTPURLResponse localizedStringForStatusCode:response.statusCode], (long)response.statusCode],
                                               NSURLErrorFailingURLErrorKey:[response URL],
                                               AFNetworkingOperationFailingURLResponseErrorKey: response,
                                       } mutableCopy];

            if (data) {
                mutableUserInfo[AFNetworkingOperationFailingURLResponseDataErrorKey] = data;
            }
            // 设置错误，通过AFErrorWithUnderlyingError这个函数设置validationError的NSUnderlyingErrorKey
            validationError = AFErrorWithUnderlyingError([NSError errorWithDomain:AFURLResponseSerializationErrorDomain code:NSURLErrorBadServerResponse userInfo:mutableUserInfo], validationError);

            responseIsValid = NO;
        }
    }
    // 赋值
    if (error && !responseIsValid) {
        *error = validationError;
    }

    return responseIsValid;
}
// *** 这个方法就是检测响应的有效性的。默认是YES。整个方法中比较值得关注的是对NSError的使用
// *** 当然我们也可以自定义key来操作NSError。在上边的方法中，有可能会出现两个错误，在self.acceptableContentTypes和self.acceptableStatusCodes这两个判断中，如果都出现错误怎么办呢？这就用到了NSUnderlyingErrorKey 这个字段，它标示一个优先的错误，value为NSErro对象。通过AFErrorWithUnderlyingError()这个函数进行了转换和赋值
// ***
/** NSError 错误信息userInfo（字典）
 * NSUnderlyingErrorKey 优先错误，value为NSError
 * NSLocalizedDescriptionKey 错误描述
 * NSLocalizedFailureReasonErrorKey 错误的简要说明
 * NSLocalizedRecoverySuggestionErrorKey 修复建议
 * NSLocalizedRecoveryOptionsErrorKey 修复选项
 */

#pragma mark - AFURLResponseSerialization
// 这个数据的转换方法不做数据转换，会验证结果，然后直接返回data
- (id)responseObjectForResponse:(NSURLResponse *)response
                           data:(NSData *)data
                          error:(NSError *__autoreleasing *)error
{
    [self validateResponse:(NSHTTPURLResponse *)response data:data error:error];

    return data;
}

#pragma mark - NSSecureCoding

+ (BOOL)supportsSecureCoding {
    return YES;
}

- (instancetype)initWithCoder:(NSCoder *)decoder {
    self = [self init];
    if (!self) {
        return nil;
    }

    self.acceptableStatusCodes = [decoder decodeObjectOfClass:[NSIndexSet class] forKey:NSStringFromSelector(@selector(acceptableStatusCodes))];
    self.acceptableContentTypes = [decoder decodeObjectOfClass:[NSIndexSet class] forKey:NSStringFromSelector(@selector(acceptableContentTypes))];

    return self;
}

- (void)encodeWithCoder:(NSCoder *)coder {
    [coder encodeObject:self.acceptableStatusCodes forKey:NSStringFromSelector(@selector(acceptableStatusCodes))];
    [coder encodeObject:self.acceptableContentTypes forKey:NSStringFromSelector(@selector(acceptableContentTypes))];
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone {
    AFHTTPResponseSerializer *serializer = [[[self class] allocWithZone:zone] init];
    serializer.acceptableStatusCodes = [self.acceptableStatusCodes copyWithZone:zone];
    serializer.acceptableContentTypes = [self.acceptableContentTypes copyWithZone:zone];

    return serializer;
}
// *** 这几个是对 NSCopying NSSecureCoding 这两个协议的实现部分，算是固定写法吧

@end

#pragma mark -

@implementation AFJSONResponseSerializer

+ (instancetype)serializer {
    return [self serializerWithReadingOptions:(NSJSONReadingOptions)0];
}

+ (instancetype)serializerWithReadingOptions:(NSJSONReadingOptions)readingOptions {
    AFJSONResponseSerializer *serializer = [[self alloc] init];
    serializer.readingOptions = readingOptions;

    return serializer;
}

- (instancetype)init {
    self = [super init];
    if (!self) {
        return nil;
    }

    self.acceptableContentTypes = [NSSet setWithObjects:@"application/json", @"text/json", @"text/javascript", nil];

    return self;
}
// *** 通过初始化方法可以看出来，AFNetworking支持的ContentType有: application/json text/json text/javascript

#pragma mark - AFURLResponseSerialization

- (id)responseObjectForResponse:(NSURLResponse *)response
                           data:(NSData *)data
                          error:(NSError *__autoreleasing *)error
{
    // 判空处理，如果验证结果失败，在没有error 或者 错误中code：NSURLErrorCannotDecodeContentData 的情况下，是不能解析数据的，就返回nil
    if (![self validateResponse:(NSHTTPURLResponse *)response data:data error:error]) {
        if (!error || AFErrorOrUnderlyingErrorHasCodeInDomain(*error, NSURLErrorCannotDecodeContentData, AFURLResponseSerializationErrorDomain)) {
            return nil;
        }
    }

    // Workaround for behavior of Rails to return a single space for `head :ok` (a workaround for a bug in Safari), which is not interpreted as valid input by NSJSONSerialization.
    // See https://github.com/rails/rails/issues/1742
    BOOL isSpace = [data isEqualToData:[NSData dataWithBytes:" " length:1]];
    
    if (data.length == 0 || isSpace) {
        return nil;
    }
    
    NSError *serializationError = nil;
    
    id responseObject = [NSJSONSerialization JSONObjectWithData:data options:self.readingOptions error:&serializationError];

    if (!responseObject)
    {
        if (error) {
            *error = AFErrorWithUnderlyingError(serializationError, *error);
        }
        return nil;
    }
    
    if (self.removesKeysWithNullValues) {
        return AFJSONObjectByRemovingKeysWithNullValues(responseObject, self.readingOptions);
    }

    return responseObject;
}

#pragma mark - NSSecureCoding

- (instancetype)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (!self) {
        return nil;
    }

    self.readingOptions = [[decoder decodeObjectOfClass:[NSNumber class] forKey:NSStringFromSelector(@selector(readingOptions))] unsignedIntegerValue];
    self.removesKeysWithNullValues = [[decoder decodeObjectOfClass:[NSNumber class] forKey:NSStringFromSelector(@selector(removesKeysWithNullValues))] boolValue];

    return self;
}

- (void)encodeWithCoder:(NSCoder *)coder {
    [super encodeWithCoder:coder];

    [coder encodeObject:@(self.readingOptions) forKey:NSStringFromSelector(@selector(readingOptions))];
    [coder encodeObject:@(self.removesKeysWithNullValues) forKey:NSStringFromSelector(@selector(removesKeysWithNullValues))];
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone {
    AFJSONResponseSerializer *serializer = [super copyWithZone:zone];
    serializer.readingOptions = self.readingOptions;
    serializer.removesKeysWithNullValues = self.removesKeysWithNullValues;

    return serializer;
}

@end

#pragma mark -

@implementation AFXMLParserResponseSerializer

+ (instancetype)serializer {
    AFXMLParserResponseSerializer *serializer = [[self alloc] init];

    return serializer;
}

- (instancetype)init {
    self = [super init];
    if (!self) {
        return nil;
    }

    self.acceptableContentTypes = [[NSSet alloc] initWithObjects:@"application/xml", @"text/xml", nil];

    return self;
}
// *** 支持的ContentType: application/xml text/xml

#pragma mark - AFURLResponseSerialization

- (id)responseObjectForResponse:(NSHTTPURLResponse *)response
                           data:(NSData *)data
                          error:(NSError *__autoreleasing *)error
{
    if (![self validateResponse:(NSHTTPURLResponse *)response data:data error:error]) {
        if (!error || AFErrorOrUnderlyingErrorHasCodeInDomain(*error, NSURLErrorCannotDecodeContentData, AFURLResponseSerializationErrorDomain)) {
            return nil;
        }
    }
    // 直接返回NSXMLParser对象
    return [[NSXMLParser alloc] initWithData:data];
}

@end

#pragma mark -

#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED

@implementation AFXMLDocumentResponseSerializer

+ (instancetype)serializer {
    return [self serializerWithXMLDocumentOptions:0];
}

+ (instancetype)serializerWithXMLDocumentOptions:(NSUInteger)mask {
    AFXMLDocumentResponseSerializer *serializer = [[self alloc] init];
    serializer.options = mask;

    return serializer;
}

- (instancetype)init {
    self = [super init];
    if (!self) {
        return nil;
    }

    self.acceptableContentTypes = [[NSSet alloc] initWithObjects:@"application/xml", @"text/xml", nil];

    return self;
}
// *** 支持的ContentType: application/xml text/xml

#pragma mark - AFURLResponseSerialization

- (id)responseObjectForResponse:(NSURLResponse *)response
                           data:(NSData *)data
                          error:(NSError *__autoreleasing *)error
{
    if (![self validateResponse:(NSHTTPURLResponse *)response data:data error:error]) {
        if (!error || AFErrorOrUnderlyingErrorHasCodeInDomain(*error, NSURLErrorCannotDecodeContentData, AFURLResponseSerializationErrorDomain)) {
            return nil;
        }
    }

    NSError *serializationError = nil;
    NSXMLDocument *document = [[NSXMLDocument alloc] initWithData:data options:self.options error:&serializationError];

    if (!document)
    {
        if (error) {
            *error = AFErrorWithUnderlyingError(serializationError, *error);
        }
        return nil;
    }
    
    return document;
}

#pragma mark - NSSecureCoding

- (instancetype)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (!self) {
        return nil;
    }

    self.options = [[decoder decodeObjectOfClass:[NSNumber class] forKey:NSStringFromSelector(@selector(options))] unsignedIntegerValue];

    return self;
}

- (void)encodeWithCoder:(NSCoder *)coder {
    [super encodeWithCoder:coder];

    [coder encodeObject:@(self.options) forKey:NSStringFromSelector(@selector(options))];
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone {
    AFXMLDocumentResponseSerializer *serializer = [super copyWithZone:zone];
    serializer.options = self.options;

    return serializer;
}

@end

#endif

#pragma mark -

@implementation AFPropertyListResponseSerializer // 这个分类用来把json解析成PropertyList

+ (instancetype)serializer {
    return [self serializerWithFormat:NSPropertyListXMLFormat_v1_0 readOptions:0];
}

+ (instancetype)serializerWithFormat:(NSPropertyListFormat)format
                         readOptions:(NSPropertyListReadOptions)readOptions
{
    AFPropertyListResponseSerializer *serializer = [[self alloc] init];
    serializer.format = format;
    serializer.readOptions = readOptions;

    return serializer;
}

- (instancetype)init {
    self = [super init];
    if (!self) {
        return nil;
    }

    self.acceptableContentTypes = [[NSSet alloc] initWithObjects:@"application/x-plist", nil];

    return self;
}
// 支持的ContentType: application/x-plist

#pragma mark - AFURLResponseSerialization

- (id)responseObjectForResponse:(NSURLResponse *)response
                           data:(NSData *)data
                          error:(NSError *__autoreleasing *)error
{
    if (![self validateResponse:(NSHTTPURLResponse *)response data:data error:error]) {
        if (!error || AFErrorOrUnderlyingErrorHasCodeInDomain(*error, NSURLErrorCannotDecodeContentData, AFURLResponseSerializationErrorDomain)) {
            return nil;
        }
    }

    if (!data) {
        return nil;
    }
    
    NSError *serializationError = nil;
    
    id responseObject = [NSPropertyListSerialization propertyListWithData:data options:self.readOptions format:NULL error:&serializationError];
    
    if (!responseObject)
    {
        if (error) {
            *error = AFErrorWithUnderlyingError(serializationError, *error);
        }
        return nil;
    }

    return responseObject;
}

#pragma mark - NSSecureCoding

- (instancetype)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (!self) {
        return nil;
    }

    self.format = (NSPropertyListFormat)[[decoder decodeObjectOfClass:[NSNumber class] forKey:NSStringFromSelector(@selector(format))] unsignedIntegerValue];
    self.readOptions = [[decoder decodeObjectOfClass:[NSNumber class] forKey:NSStringFromSelector(@selector(readOptions))] unsignedIntegerValue];

    return self;
}

- (void)encodeWithCoder:(NSCoder *)coder {
    [super encodeWithCoder:coder];

    [coder encodeObject:@(self.format) forKey:NSStringFromSelector(@selector(format))];
    [coder encodeObject:@(self.readOptions) forKey:NSStringFromSelector(@selector(readOptions))];
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone {
    AFPropertyListResponseSerializer *serializer = [super copyWithZone:zone];
    serializer.format = self.format;
    serializer.readOptions = self.readOptions;

    return serializer;
}

@end

#pragma mark -

#if TARGET_OS_IOS || TARGET_OS_TV || TARGET_OS_WATCH
#import <CoreGraphics/CoreGraphics.h>
#import <UIKit/UIKit.h>

@interface UIImage (AFNetworkingSafeImageLoading)
+ (UIImage *)af_safeImageWithData:(NSData *)data;
@end

static NSLock* imageLock = nil;

@implementation UIImage (AFNetworkingSafeImageLoading)

+ (UIImage *)af_safeImageWithData:(NSData *)data {
    UIImage* image = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        imageLock = [[NSLock alloc] init];
    });
    
    [imageLock lock];
    image = [UIImage imageWithData:data];
    [imageLock unlock];
    return image;
}
// *** 这是一个UIImage分类，添加了一个安全的NData转UIImage的方法；那么什么叫安全的呢？当我们访问或者操纵数据的时候，由于数据还可能被别人操纵，这就有可能出现不安全的情况，为了解决这个问题，引入NSLock这个锁对象

@end

/**
 * 这个私有方法，NSData -> UIImage 返回一个按照比例收缩的图片
 */
static UIImage * AFImageWithDataAtScale(NSData *data, CGFloat scale) {
    UIImage *image = [UIImage af_safeImageWithData:data];
    if (image.images) {
        return image;
    }
    
    return [[UIImage alloc] initWithCGImage:[image CGImage] scale:scale orientation:image.imageOrientation];
}

// 根据响应结果和scale返回一张图片
static UIImage * AFInflatedImageFromResponseWithDataAtScale(NSHTTPURLResponse *response, NSData *data, CGFloat scale) {
    // 1. 判空
    if (!data || [data length] == 0) {
        return nil;
    }
    // 2. 但凡带CG开头的，标示是CoreGraphics
    CGImageRef imageRef = NULL;
    // 3. CGDataProviderRef 可以细节为CoreGraphics中对data的包装
    CGDataProviderRef dataProvider = CGDataProviderCreateWithCFData((__bridge CFDataRef)data);
    // 判断响应返回的MIMEType类型
    if ([response.MIMEType isEqualToString:@"image/png"]) {
        imageRef = CGImageCreateWithPNGDataProvider(dataProvider,  NULL, true, kCGRenderingIntentDefault);
    } else if ([response.MIMEType isEqualToString:@"image/jpeg"]) {
        imageRef = CGImageCreateWithJPEGDataProvider(dataProvider, NULL, true, kCGRenderingIntentDefault);

        if (imageRef) {
            CGColorSpaceRef imageColorSpace = CGImageGetColorSpace(imageRef);
            CGColorSpaceModel imageColorSpaceModel = CGColorSpaceGetModel(imageColorSpace);

            // CGImageCreateWithJPEGDataProvider does not properly handle CMKY, so fall back to AFImageWithDataAtScale
            if (imageColorSpaceModel == kCGColorSpaceModelCMYK) {
                CGImageRelease(imageRef);
                imageRef = NULL;
            }
        }
    }

    CGDataProviderRelease(dataProvider);

    UIImage *image = AFImageWithDataAtScale(data, scale);
    if (!imageRef) {
        if (image.images || !image) {
            return image;
        }

        imageRef = CGImageCreateCopy([image CGImage]);
        if (!imageRef) {
            return nil;
        }
    }
    // 代码到了这里，这个imageRef肯定有值了，有可能是response.MIMEType得到的，也有可能是根据image得到的
    size_t width = CGImageGetWidth(imageRef);
    size_t height = CGImageGetHeight(imageRef);
    size_t bitsPerComponent = CGImageGetBitsPerComponent(imageRef);
    // 这行代码不太明白什么意思。。
    if (width * height > 1024 * 1024 || bitsPerComponent > 8) {
        CGImageRelease(imageRef);

        return image;
    }

    // CGImageGetBytesPerRow() calculates incorrectly in iOS 5.0, so defer to CGBitmapContextCreate
    size_t bytesPerRow = 0;
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGColorSpaceModel colorSpaceModel = CGColorSpaceGetModel(colorSpace);
    CGBitmapInfo bitmapInfo = CGImageGetBitmapInfo(imageRef);

    if (colorSpaceModel == kCGColorSpaceModelRGB) {
        uint32_t alpha = (bitmapInfo & kCGBitmapAlphaInfoMask);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wassign-enum"
        if (alpha == kCGImageAlphaNone) {
            bitmapInfo &= ~kCGBitmapAlphaInfoMask;
            bitmapInfo |= kCGImageAlphaNoneSkipFirst;
        } else if (!(alpha == kCGImageAlphaNoneSkipFirst || alpha == kCGImageAlphaNoneSkipLast)) {
            bitmapInfo &= ~kCGBitmapAlphaInfoMask;
            bitmapInfo |= kCGImageAlphaPremultipliedFirst;
        }
#pragma clang diagnostic pop
    }

    CGContextRef context = CGBitmapContextCreate(NULL, width, height, bitsPerComponent, bytesPerRow, colorSpace, bitmapInfo);

    CGColorSpaceRelease(colorSpace);

    if (!context) {
        CGImageRelease(imageRef);

        return image;
    }

    CGContextDrawImage(context, CGRectMake(0.0f, 0.0f, width, height), imageRef);
    CGImageRef inflatedImageRef = CGBitmapContextCreateImage(context);

    CGContextRelease(context);

    UIImage *inflatedImage = [[UIImage alloc] initWithCGImage:inflatedImageRef scale:scale orientation:image.imageOrientation];

    CGImageRelease(inflatedImageRef);
    CGImageRelease(imageRef);

    return inflatedImage;
}
// *** 上边的这个方法设计到了CoreGraphics的知识
/** 对于以上函数不明白的地方做出解释
 AFJSONResponseSerializer使用系统内置的NSJSONSerialization解析json，NSJSON只支持解析UTF8编码的数据（还有UTF-16LE之类的，都不常用），所以要先把返回的数据转成UTF8格式。这里会尝试用HTTP返回的编码类型和自己设置的stringEncoding去把数据解码转成字符串NSString，再把NSString用UTF8编码转成NSData，再用NSJSONSerialization解析成对象返回。
 
 上述过程是NSData->NSString->NSData->NSObject，这里有个问题，如果你能确定服务端返回的是UTF8编码的json数据，那NSData->NSString->NSData这两步就是无意义的，而且这两步进行了两次编解码，很浪费性能，所以如果确定服务端返回utf8编码数据，就建议自己再写个JSONResponseSerializer，跳过这两个步骤。
 
 此外AFJSONResponseSerializer专门写了个方法去除NSNull，直接把对象里值是NSNull的键去掉，还蛮贴心，若不去掉，上层很容易忽略了这个数据类型，判断了数据是否nil没判断是否NSNull，进行了错误的调用导致core。
 
 图片解压
 
 当我们调用UIImage的方法imageWithData:方法把数据转成UIImage对象后，其实这时UIImage对象还没准备好需要渲染到屏幕的数据，现在的网络图像PNG和JPG都是压缩格式，需要把它们解压转成bitmap后才能渲染到屏幕上，如果不做任何处理，当你把UIImage赋给UIImageView，在渲染之前底层会判断到UIImage对象未解压，没有bitmap数据，这时会在主线程对图片进行解压操作，再渲染到屏幕上。这个解压操作是比较耗时的，如果任由它在主线程做，可能会导致速度慢UI卡顿的问题。
 
 AFImageResponseSerializer除了把返回数据解析成UIImage外，还会把图像数据解压，这个处理是在子线程（AFNetworking专用的一条线程，详见AFURLConnectionOperation），处理后上层使用返回的UIImage在主线程渲染时就不需要做解压这步操作，主线程减轻了负担，减少了UI卡顿问题。
 
 具体实现上在AFInflatedImageFromResponseWithDataAtScale里，创建一个画布，把UIImage画在画布上，再把这个画布保存成UIImage返回给上层。只有JPG和PNG才会尝试去做解压操作，期间如果解压失败，或者遇到CMKY颜色格式的jpg，或者图像太大(解压后的bitmap太占内存，一个像素3-4字节，搞不好内存就爆掉了)，就直接返回未解压的图像。
 
 另外在代码里看到iOS才需要这样手动解压，MacOS上已经有封装好的对象NSBitmapImageRep可以做这个事。
 
 关于图片解压，还有几个问题不清楚：
 
 1.本来以为调用imageWithData方法只是持有了数据，没有做解压相关的事，后来看到调用堆栈发现已经做了一些解压操作，从调用名字看进行了huffman解码，不知还会继续做到解码jpg的哪一步。
 UIImage_jpg
 
 2.以上图片手动解压方式都是在CPU进行的，如果不进行手动解压，把图片放进layer里，让底层自动做这个事，是会用GPU进行的解压的。不知用GPU解压与用CPU解压速度会差多少，如果GPU速度很快，就算是在主线程做解压，也变得可以接受了，就不需要手动解压这样的优化了，不过目前没找到方法检测GPU解压的速度。
 */
// *** 原来做这个转化的目的是为了尽量避免在主线程解压数据，因图像太大造成内存崩溃的问题
#endif


@implementation AFImageResponseSerializer

- (instancetype)init {
    self = [super init];
    if (!self) {
        return nil;
    }
    // 支持的ContentType类型
    self.acceptableContentTypes = [[NSSet alloc] initWithObjects:@"image/tiff", @"image/jpeg", @"image/gif", @"image/png", @"image/ico", @"image/x-icon", @"image/bmp", @"image/x-bmp", @"image/x-xbitmap", @"image/x-win-bitmap", nil];

#if TARGET_OS_IOS || TARGET_OS_TV
    self.imageScale = [[UIScreen mainScreen] scale];
    self.automaticallyInflatesResponseImage = YES;
#elif TARGET_OS_WATCH
    self.imageScale = [[WKInterfaceDevice currentDevice] screenScale];
    self.automaticallyInflatesResponseImage = YES;
#endif

    return self;
}

#pragma mark - AFURLResponseSerializer

- (id)responseObjectForResponse:(NSURLResponse *)response
                           data:(NSData *)data
                          error:(NSError *__autoreleasing *)error
{
    if (![self validateResponse:(NSHTTPURLResponse *)response data:data error:error]) {
        if (!error || AFErrorOrUnderlyingErrorHasCodeInDomain(*error, NSURLErrorCannotDecodeContentData, AFURLResponseSerializationErrorDomain)) {
            return nil;
        }
    }

#if TARGET_OS_IOS || TARGET_OS_TV || TARGET_OS_WATCH
    if (self.automaticallyInflatesResponseImage) {
        return AFInflatedImageFromResponseWithDataAtScale((NSHTTPURLResponse *)response, data, self.imageScale);
    } else {
        return AFImageWithDataAtScale(data, self.imageScale);
    }
#else
    // Ensure that the image is set to it's correct pixel width and height
    NSBitmapImageRep *bitimage = [[NSBitmapImageRep alloc] initWithData:data];
    NSImage *image = [[NSImage alloc] initWithSize:NSMakeSize([bitimage pixelsWide], [bitimage pixelsHigh])];
    [image addRepresentation:bitimage];

    return image;
#endif

    return nil;
}

#pragma mark - NSSecureCoding

- (instancetype)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (!self) {
        return nil;
    }

#if TARGET_OS_IOS  || TARGET_OS_TV || TARGET_OS_WATCH
    NSNumber *imageScale = [decoder decodeObjectOfClass:[NSNumber class] forKey:NSStringFromSelector(@selector(imageScale))];
#if CGFLOAT_IS_DOUBLE
    self.imageScale = [imageScale doubleValue];
#else
    self.imageScale = [imageScale floatValue];
#endif

    self.automaticallyInflatesResponseImage = [decoder decodeBoolForKey:NSStringFromSelector(@selector(automaticallyInflatesResponseImage))];
#endif

    return self;
}

- (void)encodeWithCoder:(NSCoder *)coder {
    [super encodeWithCoder:coder];

#if TARGET_OS_IOS || TARGET_OS_TV || TARGET_OS_WATCH
    [coder encodeObject:@(self.imageScale) forKey:NSStringFromSelector(@selector(imageScale))];
    [coder encodeBool:self.automaticallyInflatesResponseImage forKey:NSStringFromSelector(@selector(automaticallyInflatesResponseImage))];
#endif
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone {
    AFImageResponseSerializer *serializer = [super copyWithZone:zone];

#if TARGET_OS_IOS || TARGET_OS_TV || TARGET_OS_WATCH
    serializer.imageScale = self.imageScale;
    serializer.automaticallyInflatesResponseImage = self.automaticallyInflatesResponseImage;
#endif

    return serializer;
}

@end

#pragma mark -

@interface AFCompoundResponseSerializer ()
@property (readwrite, nonatomic, copy) NSArray *responseSerializers;
@end

@implementation AFCompoundResponseSerializer

+ (instancetype)compoundSerializerWithResponseSerializers:(NSArray *)responseSerializers {
    AFCompoundResponseSerializer *serializer = [[self alloc] init];
    serializer.responseSerializers = responseSerializers;

    return serializer;
}

#pragma mark - AFURLResponseSerialization

- (id)responseObjectForResponse:(NSURLResponse *)response
                           data:(NSData *)data
                          error:(NSError *__autoreleasing *)error
{
    for (id <AFURLResponseSerialization> serializer in self.responseSerializers) {
        if (![serializer isKindOfClass:[AFHTTPResponseSerializer class]]) {
            continue;
        }

        NSError *serializerError = nil;
        id responseObject = [serializer responseObjectForResponse:response data:data error:&serializerError];
        if (responseObject) {
            if (error) {
                *error = AFErrorWithUnderlyingError(serializerError, *error);
            }

            return responseObject;
        }
    }

    return [super responseObjectForResponse:response data:data error:error];
}
// *** 这个复合型的子类有一个数组属性，里边装着多种序列化类型。看其实现方法也是，遍历数组中的内容，只要能转化成功，就返回数据。

#pragma mark - NSSecureCoding

- (instancetype)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (!self) {
        return nil;
    }

    self.responseSerializers = [decoder decodeObjectOfClass:[NSArray class] forKey:NSStringFromSelector(@selector(responseSerializers))];

    return self;
}

- (void)encodeWithCoder:(NSCoder *)coder {
    [super encodeWithCoder:coder];

    [coder encodeObject:self.responseSerializers forKey:NSStringFromSelector(@selector(responseSerializers))];
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone {
    AFCompoundResponseSerializer *serializer = [super copyWithZone:zone];
    serializer.responseSerializers = self.responseSerializers;

    return serializer;
}

// *** 本节学习内容
// *** 通过一个协议来得到不同转换的结果。
// *** 知道了AFNetworking响应结果支持的各种类型。
// *** 大体了解了NSIndexSet的使用方法
// *** 如何创建一个NSError 和带有优先错误的NSUnderlyingErrorKey
// *** 服务器返回的图片是压缩格式，要进行解压
// *** 使用images来实现gif效果

@end
