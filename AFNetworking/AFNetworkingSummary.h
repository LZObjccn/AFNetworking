//  https://www.cnblogs.com/machao/p/5830337.html
//  AFNetworkingSummary.h
//  AFNetworking iOS
//
//  Created by lizi' zhen on 2019/6/29.
//  Copyright © 2019 AFNetworking. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface AFNetworkingSummary : NSObject

/**
 * 1.枚举(enum)
     使用原则：当满足一个有限的并具有统一主题的集合的时候，我们就考虑使用枚举。这在很多框架中都验证了这个原则。最重要的是能够增加程序的可读性。
     示例代码：
        // 网络类型 （需要封装为一个自己的枚举）
        typedef NS_ENUM(NSInteger, AFNetworkReachabilityStatus) {
            // 未知
            AFNetworkReachabilityStatusUnknown          = -1,
            // 无网络
            AFNetworkReachabilityStatusNotReachable     = 0,
            // WWAN 手机自带网络
            AFNetworkReachabilityStatusReachableViaWWAN = 1,
            // WiFi
            AFNetworkReachabilityStatusReachableViaWiFi = 2,
        };
 
 * 2. 注释
     我们必须知道一个事实，注释的代码是不会编译到目标文件的，因此放心大胆的注释吧。在平日里的开发中，应该经常问问自己是否把每段代码都当成写API那样对待？
     曾经看过两种不同的说辞，一种是说把代码注释尽量少些，要求代码简介可读性强。另一种是说注释要详细，着重考虑他人读代码的感受。个人感觉还是写详细一点比较好，因为可能过一段时间之后，自己再去看自己当时写的代码可能就不记得了。很有可能在写这些繁琐的注释的过程中，能够想到些什么，比如如何合并掉一些没必要的方法等等。
 
 * 3. BOOL属性的property书写规则
     通常我们在定义一个BOOL属性的时候，要自定义getter方法，这样做的目的是为了增加程序的可读性。Apple中的代码也是这么写的。
     示例代码：
         // Whether or not the network is currently reachable.
        @property (readonly, nonatomic, assign, getter = isReachable) BOOL reachable;

        // setter
        self.reachable = YES;
        // getter
        if (self.isReachable) {}
 
 * 4. 按功能区分代码
     假如我们写的一个控制器中大概有500行代码，我们应该保证能够快速的找到我们需要查找的内容，这就需要把代码按照功能来分隔。
     通常在.h中 我们可以使用一个自定义的特殊的注释来分隔，在.m中使用#pragma mark -来分隔。
     示例代码:
         ///---------------------
         /// @name Initialization
         ///---------------------
 
         ///------------------------------
         /// @name Evaluating Server Trust
         ///------------------------------
 
         #pragma mark - UI
         ...设置UI相关
         #pragma mark - Data
         ...处理数据
         #pragma mark - Action
         ...点击事件
 * 5. 通知
     我们都知道通知可以用来传递事件和数据，但要想用好它，也不太容易。在 AFNetworking 事件和数据的传递使用的是通知和Block，按照AFNetworking对通知的使用习惯。我总结了几点：
     原则：如果我们需要传递事件或数据，可采用代理和Block，同时额外增加一个通知。因为通知具有跨多个界面的优点。
     释放问题：在接收通知的页面，一定要记得移除监听。
     使用方法：在.h中 FOUNDATION_EXPORT + NSString * const +通知名 在.m中赋值。如果在别的页面用到这个通知，使用extern + NSString * const +通知名就可以了。
 
    ps： FOUNDATION_EXPORT 和#define 都能定义常量。FOUNDATION_EXPORT 能够使用==进行判断，效率略高。而且能够隐藏定义细节(就是实现部分不在.中)
    示例代码:
         FOUNDATION_EXPORT NSString * const AFNetworkingReachabilityDidChangeNotification;
         FOUNDATION_EXPORT NSString * const AFNetworkingReachabilityNotificationStatusItem;
         // 网络环境发生改变的时候接受的通知
         NSString * const AFNetworkingReachabilityDidChangeNotification = @"com.alamofire.networking.reachability.change";
         // 网络环境发生变化是会发送一个通知，同时携带一组状态数据，根据这个key来去除网络status
         NSString * const AFNetworkingReachabilityNotificationStatusItem = @"AFNetworkingReachabilityNotificationStatusItem";

 * 6. 国际化
     我个人认为在开发一个APP之初，就应该考虑国际化的问题，不管日后会不会用到这个功能。当你有了国际化的思想之后，在对控件进行布局的时候，就会比只在一种语言下考虑的更多，这会让一个人对控件布局的视野更加宽阔。好了，这个问题就说这么多。有兴趣的朋友请自行查找相关内容。
 * 7. 私有方法
     在开发中，难免会使用私有方法来协助我们达到某种目的或获取某个数据。在oc中，我看到很多人都会这样写：- (void)funName {}。个人是不赞成这样写了，除非方法内部使用了self。总之，类似于这样的方法，其实跟我们的业务并没有太大的关系。我进入一个控制器的文件中，目光应该集中在业务代码上才对。
 
     在 AFNetworking 中，一般都会把私有方法，也可以叫函数，放到头部，你即使不看这些代码，对于整个业务的理解也不会受到影响。所以，这种写法值得推荐。可以适当的使用内联函数，提高效率.
     示例代码:
        // 把枚举的值转换成字符串
        NSString * AFStringFromNetworkReachabilityStatus(AFNetworkReachabilityStatus status) {
            switch (status) {
                case AFNetworkReachabilityStatusNotReachable:
                    return NSLocalizedStringFromTable(@"Not Reachable", @"AFNetworking", nil);
                case AFNetworkReachabilityStatusReachableViaWWAN:
                    return NSLocalizedStringFromTable(@"Reachable via WWAN", @"AFNetworking", nil);
                case AFNetworkReachabilityStatusReachableViaWiFi:
                    return NSLocalizedStringFromTable(@"Reachable via WiFi", @"AFNetworking", nil);
                case AFNetworkReachabilityStatusUnknown:
                default:
                    return NSLocalizedStringFromTable(@"Unknown", @"AFNetworking", nil);
            }
        }
 
 * 8. SCNetworkReachabilityRef（网络监控核心实现）
     SCNetworkReachabilityRef 是获取网络状态的核心对象，创建这个对象有两个方法：
     --> SCNetworkReachabilityCreateWithName
     --> SCNetworkReachabilityCreateWithAddress
     我们看看实现网络监控的核心代码：
     示例代码:
         - (void)startMonitoring {
         [self stopMonitoring];
         if (!self.networkReachability) {
            return;
         }
         __weak __typeof(self)weakSelf = self;
         AFNetworkReachabilityStatusBlock callback = ^(AFNetworkReachabilityStatus status) {
             __strong __typeof(weakSelf)strongSelf = weakSelf;
 
             strongSelf.networkReachabilityStatus = status;
             if (strongSelf.networkReachabilityStatusBlock) {
                strongSelf.networkReachabilityStatusBlock(status);
             }
         };
         SCNetworkReachabilityContext context = {0, (__bridge void *)callback, AFNetworkReachabilityRetainCallback, AFNetworkReachabilityReleaseCallback, NULL};
         SCNetworkReachabilitySetCallback(self.networkReachability, AFNetworkReachabilityCallback, &context);
         SCNetworkReachabilityScheduleWithRunLoop(self.networkReachability, CFRunLoopGetMain(), kCFRunLoopCommonModes);
         dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0),^{
             SCNetworkReachabilityFlags flags;
             if (SCNetworkReachabilityGetFlags(self.networkReachability, &flags)) {
                 AFPostReachabilityStatusChange(flags, callback);
                 }
             });
         }
     上边的方法中涉及了一些 CoreFoundation 的知识，我们来看看:
     SCNetworkReachabilityContext点进去，会发现这是一个结构体，一般c语言的结构体是对要保存的数据的一种描述
    示例代码：
         typedef struct {
             CFIndex     version;
             void *      __nullable info;
             const void  * __nonnull (* __nullable retain)(const void *info);
             void        (* __nullable release)(const void *info);
             CFStringRef __nonnull (* __nullable copyDescription)(const void *info);
         } SCNetworkReachabilityContext;
         第一个参数接受一个signed long 的参数
         第二个参数接受一个void * 类型的值，相当于oc的id类型，void * 可以指向任何类型的参数
         第三个参数 是一个函数 目的是对info做retain操作
         第四个参数是一个函数，目的是对info做release操作
         第五个参数是 一个函数，根据info获取Description字符串
     设置网络监控分为下边几个步骤：
     1.我们先新建上下文
     SCNetworkReachabilityContext context = {0, (__bridge void *)callback, AFNetworkReachabilityRetainCallback, AFNetworkReachabilityReleaseCallback, NULL};
     2.设置回调
     SCNetworkReachabilitySetCallback(self.networkReachability, AFNetworkReachabilityCallback, &context);
     3.加入RunLoop池
     SCNetworkReachabilityScheduleWithRunLoop(self.networkReachability, CFRunLoopGetMain(), kCFRunLoopCommonModes);
 
 * 9. 键值依赖
    注册键值依赖，这个可能大家平时用的比较少。可以了解一下
    示例代码：
         @interface User :NSObject
         @property (nonatomic,copy)NSString *name;
         @property (nonatomic,assign)NSUInteger age;
         @end
 
         @interface card :NSObject
         @property (nonatomic,copy)NSString *info;
         @property (nonatomic,strong)User *user;
         @end
         @implementation card
 
         - (NSString *)info {
            return [NSString stringWithFormat:@"%@/%lu",_user.name,(unsigned long)_user.age];
         }
         - (void)setInfo:(NSString *)info {
             NSArray *array = [info componentsSeparatedByString:@"/"];
             _user.name = array[0];
             _user.age = [array[1] integerValue];
         }
 
         + (NSSet<NSString *> *)keyPathsForValuesAffectingValueForKey:(NSString *)key {
             NSSet * keyPaths = [super keyPathsForValuesAffectingValueForKey:key];
             NSArray * moreKeyPaths = nil;
 
             if ([key isEqualToString:@"info"])
             {
                moreKeyPaths = [NSArray arrayWithObjects:@"user.name", @"user.age", nil];
             }
 
             if (moreKeyPaths)
             {
                keyPaths = [keyPaths setByAddingObjectsFromArray:moreKeyPaths];
             }
 
            return keyPaths;
         }
 
         @end
 
 * 10. HTTP
     --> HTTP协议用于客户端和服务器端之间的通信
     --> 通过请求和相应的交换达成通信
     --> HTTP是不保存状态的协议
        HTTP自身不会对请求和相应之间的通信状态进行保存。什么意思呢？就是说，当有新的请求到来的时候，HTTP就会产生新的响应，对之前的请求和响应的保温信息不做任何存储。这也是为了快速的处理事务，保持良好的可伸展性而特意设计成这样的。
     --> 请求URI定位资源
        URI算是一个位置的索引，这样就能很方便的访问到互联网上的各种资源。
     --> 告知服务器意图的HTTP方法
         ①GET： 直接访问URI识别的资源，也就是说根据URI来获取资源。
         ②POST： 用来传输实体的主体。
         ③PUT： 用来传输文件。
         ④HEAD： 用来获取报文首部，和GET方法差不多，只是响应部分不会返回主体内容。
         ⑤DELETE： 删除文件，和PUT恰恰相反。按照请求的URI来删除指定位置的资源。
         ⑥OPTIONS： 询问支持的方法，用来查询针对请求URI指定的资源支持的方法。
         ⑦TRACE： 追踪路径，返回服务器端之前的请求通信环信息。
         ⑧CONNECT： 要求用隧道协议连接代理，要求在与代理服务器通信时建立隧道，实现用隧道协议进行TCP通信。SSL(Secure Sockets Layer)和TLS(Transport Layer Security)就是把通信内容加密后进行隧道传输的。
     --> 管线化让服务器具备了相应多个请求的能力
     --> Cookie让HTTP有迹可循
 
 * 11. HTTPS
     HTTPS是一个通信安全的解决方案，可以说相对已经非常安全。为什么它会是一个很安全的协议呢？下边会做出解释。大家可以看看这篇文章，解释的很有意思 。
     HTTP + 加密 + 认证 + 完整性保护 = HTTPS
     其实HTTPS是身披SSL外壳的HTTP，这句话怎么理解呢？
     大家应该都知道HTTP是应用层的协议，但HTTPS并非是应用层的一种新协议，只是HTTP通信接口部分用SSL或TLS协议代替而已。
     通常 HTTP 直接和TCP通信，当使用SSL时就不同了。要先和SSL通信，再由SSL和TCP通信。
 
     介绍两种常用加密方法：
         --> 共享密钥加密
         --> 公开密钥加密
     共享密钥加密就是加密和解密通用一个密钥，也称为对称加密。优点是加密解密速度快，缺点是一旦密钥泄露，别人也能解密数据。
     公开密钥加密恰恰能解决共享密钥加密的困难，过程是这样的：
         ①发文方使用对方的公开密钥进行加密
         ②接受方在使用自己的私有密钥进行解密
     关于公开密钥，也就是非对称加密 可以看看这篇文章 RSA算法原理
     原理都是一样的，这个不同于刚才举得a和b的例子，就算知道了结果和公钥，破解出被机密的数据是非常难的。这里边主要涉及到了复杂的数学理论。
     HTTPS采用混合加密机制
     HTTPS采用共享密钥加密和公开密钥加密两者并用的混合加密机制。
 
 * 12. 如何获取证书中的PublicKey
     // 在证书中获取公钥
     static id AFPublicKeyForCertificate(NSData *certificate) {
         id allowedPublicKey = nil;
         SecCertificateRef allowedCertificate;
         SecCertificateRef allowedCertificates[1];
         CFArrayRef tempCertificates = nil;
         SecPolicyRef policy = nil;
         SecTrustRef allowedTrust = nil;
         SecTrustResultType result;
 
         // 1. 根据二进制的certificate生成SecCertificateRef类型的证书
         // NSData *certificate 通过CoreFoundation (__bridge CFDataRef)转换成 CFDataRef
         // 看下边的这个方法就可以知道需要传递参数的类型
         // SecCertificateRef SecCertificateCreateWithData(CFAllocatorRef __nullable allocator,
         // CFDataRef data) __OSX_AVAILABLE_STARTING(__MAC_10_6, __IPHONE_2_0);
         allowedCertificate = SecCertificateCreateWithData(NULL, (__bridge CFDataRef)certificate);
         // 2.如果allowedCertificate为空，则执行标记_out后边的代码
         __Require_Quiet(allowedCertificate != NULL, _out);
         // 3.给allowedCertificates赋值
         allowedCertificates[0] = allowedCertificate;
         // 4.新建CFArra： tempCertificates
         tempCertificates = CFArrayCreate(NULL, (const void **)allowedCertificates, 1, NULL);
         // 5. 新建policy为X.509
         policy = SecPolicyCreateBasicX509();
         // 6.创建SecTrustRef对象，如果出错就跳到_out标记处
         __Require_noErr_Quiet(SecTrustCreateWithCertificates(tempCertificates, policy, &allowedTrust), _out);
         // 7.校验证书的过程，这个不是异步的。
         __Require_noErr_Quiet(SecTrustEvaluate(allowedTrust, &result), _out);
         // 8.在SecTrustRef对象中取出公钥
         allowedPublicKey = (__bridge_transfer id)SecTrustCopyPublicKey(allowedTrust);
         _out:
         if (allowedTrust) {
                CFRelease(allowedTrust);
         }
         if (policy) {
                CFRelease(policy);
         }
         if (tempCertificates) {
                CFRelease(tempCertificates);
         }
         if (allowedCertificate) {
                CFRelease(allowedCertificate);
         }
         return allowedPublicKey;
     }
     在二进制的文件中获取公钥的过程是这样
     ① NSData *certificate -> CFDataRef -> (SecCertificateCreateWithData) -> SecCertificateRef allowedCertificate
     ②判断SecCertificateRef allowedCertificate 是不是空，如果为空，直接跳转到后边的代码
     ③allowedCertificate 保存在allowedCertificates数组中
     ④allowedCertificates -> (CFArrayCreate) -> SecCertificateRef allowedCertificates[1]
     ⑤根据函数SecPolicyCreateBasicX509() -> SecPolicyRef policy
     ⑥SecTrustCreateWithCertificates(tempCertificates, policy, &allowedTrust) -> 生成SecTrustRef allowedTrust
     ⑦SecTrustEvaluate(allowedTrust, &result) 校验证书
     ⑧(__bridge_transfer id)SecTrustCopyPublicKey(allowedTrust) -> 得到公钥id allowedPublicKey
     这个过程我们平时也不怎么用，了解下就行了，真需要的时候知道去哪里找资料就行了。
     这里边值得学习的地方是：
     __Require_Quiet 和 __Require_noErr_Quiet 这两个宏定义。
 
 * 13. URL编码
 * 14. HTTPBody
 * 15. 保证方法在主线程执行
     - (BOOL)transitionToNextPhase {
         // 保证代码在主线程
         if (![[NSThread currentThread] isMainThread]) {
             dispatch_sync(dispatch_get_main_queue(), ^{
                 [self transitionToNextPhase];
             });
             return YES;
         }
     }
 * 16. 代码跟思想的碰撞
     - (BOOL)transitionToNextPhase {
 
         // 保证代码在主线程
         if (![[NSThread currentThread] isMainThread]) {
             dispatch_sync(dispatch_get_main_queue(), ^{
                [self transitionToNextPhase];
             });
             return YES;
         }
 
         #pragma clang diagnostic push
         #pragma clang diagnostic ignored "-Wcovered-switch-default"
         switch (_phase) {
             case AFEncapsulationBoundaryPhase:
                 _phase = AFHeaderPhase;
                 break;
             case AFHeaderPhase:  // 打开流，准备接受数据
                 [self.inputStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
                 [self.inputStream open];
                 _phase = AFBodyPhase;
                 break;
             case AFBodyPhase: // 关闭流
                 [self.inputStream close];
                 _phase = AFFinalBoundaryPhase;
                 break;
             case AFFinalBoundaryPhase:
             default:
                 _phase = AFEncapsulationBoundaryPhase;
                 break;
         }
         // 重置offset
         _phaseReadOffset = 0;
         #pragma clang diagnostic pop
 
         return YES;
     }
     回过头来看这段代码，我又有新的想法。原本对数据的操作，对body的操作，是一件很复杂的事情。但作者的思路非常清晰。就像上边这个方法一样，它只实现一个功能，就是切换body组成部分。它只做了这一件事，我们在开发中，如遇到有些复杂的功能，在写方法的时候，可能考虑了很多东西，当时所有的考虑可能都写到一个方法中了。
      能不能写出一个思路图，先不管思路的实现如何，先一一列出来，最后在一一实现，一一拼接起来。
 
 * 17. NSInputStream
    NSInputStream有好几种类型，根据不同的类型返回不同方法创建的NSInputStream
    示例代码:
         - (NSInputStream *)inputStream {
             if (!_inputStream) {
                 if ([self.body isKindOfClass:[NSData class]]) {
                    _inputStream = [NSInputStream inputStreamWithData:self.body];
                 } else if ([self.body isKindOfClass:[NSURL class]]) {
                    _inputStream = [NSInputStream inputStreamWithURL:self.body];
                 } else if ([self.body isKindOfClass:[NSInputStream class]]) {
                    _inputStream = self.body;
                 } else {
                    _inputStream = [NSInputStream inputStreamWithData:[NSData data]];
                 }
            }
 
             return _inputStream;
         }
 
 * 18. 对文件的操作
     NSParameterAssert() 用来判断参数是否为空，如果为空就抛出异常
     使用isFileURL 判断一个URL是否为fileURL 使用checkResourceIsReachableAndReturnError判断路径能够到达
     使用 [[NSFileManager defaultManager] attributesOfItemAtPath:[fileURL path] error:error] 获取本地文件属性
     lastPathComponent ，https://www.baidu.com/abc.html 结果就是abc.html
     pathExtension https://www.baidu.com/abc.html 结果就是html
 
 * 19. NSURLRequestCachePolicy缓存策略
    NSURLRequestUseProtocolCachePolicy 这个是默认的缓存策略，缓存不存在，就请求服务器，缓存存在，会根据response中的Cache-Control字段判断下一步操作，如: Cache-Control字段为must-revalidata, 则询问服务端该数据是否有更新，无更新的话直接返回给用户缓存数据，若已更新，则请求服务端。
     --> NSURLRequestReloadIgnoringLocalCacheData 这个策略是不管有没有本地缓存，都请求服务器。
     --> NSURLRequestReloadIgnoringLocalAndRemoteCacheData 这个策略会忽略本地缓存和中间代理 直接访问源server
     --> NSURLRequestReturnCacheDataElseLoad 这个策略指，有缓存就是用，不管其有效性，即Cache-Control字段 ，没有就访问源server
     --> NSURLRequestReturnCacheDataDontLoad 这个策略只加载本地数据，不做其他操作，适用于没有网路的情况
     --> NSURLRequestReloadRevalidatingCacheData 这个策略标示缓存数据必须得到服务器确认才能使用，未实现。
 
 * 20. 管线化
    在HTTP连接中，一般都是一个请求对应一个连接，每次建立tcp连接是需要一定时间的。管线化，允许一次发送一组请求而不必等到响应。但由于目前并不是所有的服务器都支持这项功能，因此这个属性默认是不开启的。管线化使用同一tcp连接完成任务，因此能够大大提交请求的时间。但是响应要和请求的顺序 保持一致才行。使用场景也有，比如说首页要发送很多请求，可以考虑这种技术。但前提是建立连接成功后才可以使用。

 */

@end
