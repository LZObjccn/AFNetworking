// AFNetworkReachabilityManager.h
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
// 用来监控网络环境变化的类
#import <Foundation/Foundation.h>

#if !TARGET_OS_WATCH
#import <SystemConfiguration/SystemConfiguration.h>
/**
 *  网络类型
 */
typedef NS_ENUM(NSInteger, AFNetworkReachabilityStatus) {
    // 未知
    AFNetworkReachabilityStatusUnknown          = -1,
    // 无网络
    AFNetworkReachabilityStatusNotReachable     = 0,
    // WWAN 手机自带网络
    AFNetworkReachabilityStatusReachableViaWWAN = 1,
    // WiFi
    AFNetworkReachabilityStatusReachableViaWiFi = 2,
}; // 这是一个枚举封装，还是遵循一个使用枚举的原则，当满足一个有限的并具有统一主题的集合的时候，我们就考虑枚举。在这里作者是枚举了4种类型。这几种类型能够满足我们开发中大部分的功能，如果不满足，可以自行进行扩展。

NS_ASSUME_NONNULL_BEGIN // 这个是为了swift的可选类型配添加的，在这两个终点的内容的参数默认都是nonnull的。

/**  这段文字是对这个类的说明
 `AFNetworkReachabilityManager` monitors the reachability of domains, and addresses for both WWAN and WiFi network interfaces.

 Reachability can be used to determine background information about why a network operation failed, or to trigger a network operation retrying when a connection is established. It should not be used to prevent a user from initiating a network request, as it's possible that an initial request may be required to establish reachability.

 See Apple's Reachability Sample Code ( https://developer.apple.com/library/ios/samplecode/reachability/ )

 @warning Instances of `AFNetworkReachabilityManager` must be started with `-startMonitoring` before reachability status can be determined.
 */
@interface AFNetworkReachabilityManager : NSObject

// 本类提供了四个只读的属性来让我们获取我们需要的内容
/** 网络状态
 The current network reachability status.
 */
@property (readonly, nonatomic, assign) AFNetworkReachabilityStatus networkReachabilityStatus;

/** 是否是可达的
 Whether or not the network is currently reachable.
 */
@property (readonly, nonatomic, assign, getter = isReachable) BOOL reachable;

/** 当前连接是否是WWAN
 Whether or not the network is currently reachable via WWAN.
 */
@property (readonly, nonatomic, assign, getter = isReachableViaWWAN) BOOL reachableViaWWAN;

/** 当前连接是够是WiFi
 Whether or not the network is currently reachable via WiFi.
 */
@property (readonly, nonatomic, assign, getter = isReachableViaWiFi) BOOL reachableViaWiFi;
// *** 四个属性均为只读属性，只给了用户访问权，注意BOOL属性一般是要写getter方法的。

///---------------------
/// @name Initialization(初始化) 作者使用了这个来分隔同一类中不同功能模块。这个算是个人习惯问题吧。举个平时开发的例子，在.m文件中我个人使用#pragma mark 分隔不同功能。
///---------------------

/**
 Returns the shared network reachability manager.
 */
+ (instancetype)sharedManager;

/**
 Creates and returns a network reachability manager with the default socket address.
 
 @return An initialized network reachability manager, actively monitoring the default socket address.
 */
+ (instancetype)manager;

/** 监听制定domain的网络状态。
 Creates and returns a network reachability manager for the specified domain.

 @param domain The domain used to evaluate network reachability.

 @return An initialized network reachability manager, actively monitoring the specified domain.
 */
+ (instancetype)managerForDomain:(NSString *)domain;

/** 监听某个socket地址的网络状态
 Creates and returns a network reachability manager for the socket address.

 @param address The socket address (`sockaddr_in6`) used to evaluate network reachability.

 @return An initialized network reachability manager, actively monitoring the specified socket address.
 */
+ (instancetype)managerForAddress:(const void *)address;

/**
 Initializes an instance of a network reachability manager from the specified reachability object.

 @param reachability The reachability object to monitor.

 @return An initialized network reachability manager, actively monitoring the specified reachability.
 */
- (instancetype)initWithReachability:(SCNetworkReachabilityRef)reachability NS_DESIGNATED_INITIALIZER;

/**
 *  Unavailable initializer
 */
+ (instancetype)new NS_UNAVAILABLE;

/**
 *  Unavailable initializer
 */
- (instancetype)init NS_UNAVAILABLE;

///--------------------------------------------------
/// @name Starting & Stopping Reachability Monitoring
///--------------------------------------------------

/** 打开监听
 Starts monitoring for changes in network reachability status.
 */
- (void)startMonitoring;

/** 关闭监听
 Stops monitoring for changes in network reachability status.
 */
- (void)stopMonitoring;

///-------------------------------------------------
/// @name Getting Localized Reachability Description
///-------------------------------------------------

/** 返回一个网络状态的本地语言的字符串。往往我们可以根据这个字符串来告诉用户，当前网络发生了什么，当然，也可以根据状态自定义提示文字。
 Returns a localized string representation of the current network reachability status.
 */
- (NSString *)localizedNetworkReachabilityStatusString;

///---------------------------------------------------
/// @name Setting Network Reachability Change Callback
///---------------------------------------------------

/** 设置网络状态改变的回调
 Sets a callback to be executed when the network availability of the `baseURL` host changes.

 @param block A block object to be executed when the network availability of the `baseURL` host changes.. This block has no return value and takes a single argument which represents the various reachability states from the device to the `baseURL`.
 */
- (void)setReachabilityStatusChangeBlock:(nullable void (^)(AFNetworkReachabilityStatus status))block;

@end

///----------------
/// @name Constants
///----------------

/**
 ## Network Reachability

 The following constants are provided by `AFNetworkReachabilityManager` as possible network reachability statuses.

 enum {
 AFNetworkReachabilityStatusUnknown,
 AFNetworkReachabilityStatusNotReachable,
 AFNetworkReachabilityStatusReachableViaWWAN,
 AFNetworkReachabilityStatusReachableViaWiFi,
 }

 `AFNetworkReachabilityStatusUnknown`
 The `baseURL` host reachability is not known.

 `AFNetworkReachabilityStatusNotReachable`
 The `baseURL` host cannot be reached.

 `AFNetworkReachabilityStatusReachableViaWWAN`
 The `baseURL` host can be reached via a cellular connection, such as EDGE or GPRS.

 `AFNetworkReachabilityStatusReachableViaWiFi`
 The `baseURL` host can be reached via a Wi-Fi connection.

 ### Keys for Notification UserInfo Dictionary

 Strings that are used as keys in a `userInfo` dictionary in a network reachability status change notification.

 `AFNetworkingReachabilityNotificationStatusItem`
 A key in the userInfo dictionary in a `AFNetworkingReachabilityDidChangeNotification` notification.
 The corresponding value is an `NSNumber` object representing the `AFNetworkReachabilityStatus` value for the current reachability status.
 */

///--------------------
/// @name Notifications
///--------------------

/**
 Posted when network reachability changes.
 This notification assigns no notification object. The `userInfo` dictionary contains an `NSNumber` object under the `AFNetworkingReachabilityNotificationStatusItem` key, representing the `AFNetworkReachabilityStatus` value for the current network reachability.

 @warning In order for network reachability to be monitored, include the `SystemConfiguration` framework in the active target's "Link Binary With Library" build phase, and add `#import <SystemConfiguration/SystemConfiguration.h>` to the header prefix of the project (`Prefix.pch`).
 */
// 监听网络状态变化的通知
FOUNDATION_EXPORT NSString * const AFNetworkingReachabilityDidChangeNotification;
// 接受的通知中会有一个userinfo 是一个NSDictionary 其中key就是 AFNetworkingReachabilityNotificationStatusItem
FOUNDATION_EXPORT NSString * const AFNetworkingReachabilityNotificationStatusItem;
// *** 这简单的两行代码能够告诉我们的是，我们平时的开发中 但凡设计到发通知的功能，我们应该把通知的字符串封装到一个专有的文件中，同时在文件内部按不同模块进行区分，当然必要的注释也很有必要。
// *** FOUNDATION_EXPORT 和#define 都能定义常量。FOUNDATION_EXPORT 能够使用==进行判断，效率略高。而且能够隐藏定义细节


///--------------------
/// @name Functions
///--------------------

/** 根据状态获取字符串
 Returns a localized string representation of an `AFNetworkReachabilityStatus` value.
 */
FOUNDATION_EXPORT NSString * AFStringFromNetworkReachabilityStatus(AFNetworkReachabilityStatus status);

NS_ASSUME_NONNULL_END
#endif
