// AFSecurityPolicy.h 网络连接安全(验证证书是否正确)
// https://www.cnblogs.com/machao/p/5704201.html
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
/**
 * HTTP:
 * 1.HTTP协议用于客户端和服务器端之间的通信
 * 2.通过请求和相应的交换达成通信
 * 3.HTTP是不保存状态的协议
   HTTP自身不会对请求和相应之间的通信状态进行保存。什么意思呢？就是说，当有新的请求到来的时候，HTTP就会产生新的响应，对之前的请求和响应的保温信息不做任何存储。这也是为了快速的处理事务，保持良好的可伸展性而特意设计成这样的。
 * 4.请求URI定位资源
   URI算是一个位置的索引，这样就能很方便的访问到互联网上的各种资源。
 * 5.告知服务器意图的HTTP方法
   ①GET： 直接访问URI识别的资源，也就是说根据URI来获取资源。
   ②POST： 用来传输实体的主体。
   ③PUT： 用来传输文件。
   ④HEAD： 用来获取报文首部，和GET方法差不多，只是响应部分不会返回主体内容。
   ⑤DELETE： 删除文件，和PUT恰恰相反。按照请求的URI来删除指定位置的资源。
   ⑥OPTIONS： 询问支持的方法，用来查询针对请求URI指定的资源支持的方法。
   ⑦TRACE： 追踪路径，返回服务器端之前的请求通信环信息。
   ⑧CONNECT： 要求用隧道协议连接代理，要求在与代理服务器通信时简历隧道，实现用隧道协议进行TCP通信。SSL(Secure Sockets Layer)和TLS(Transport Layer Security)就是把通信内容加密后进行隧道传输的。
 * 6.管线化让服务器具备了响应多个请求的能力
 * 7.Cookie让HTTP有迹可循
 HTTP是一套很简单通信协议，因此也非常的高效。但是由于通信数据都是明文发送的，很容易被拦截后造成破坏。在互联网越来越发达的时代，对通信数据的安全要求也越来越高。
 */

/**
 * HTTPS:
 * HTTPS是一个通信安全的解决方案，可以说相对已经非常安全。
 * HTTP + 加密 + 认证 + 完整性保护 = HTTPS, 其实HTTPS是身披SSL外壳的HTTP
 * 大家应该都知道HTTP是应用层的协议，但HTTPS并非是应用层的一种新协议，只是HTTP通信接口部分用SSL或TLS协议代替而已。
 * 通常 HTTP 直接和TCP通信，当使用SSL时就不同了。要先和SSL通信，再由SSL和TCP通信。
 * HTTPS采用共享密钥加密和公开密钥加密两者并用的混合加密机制。
 * ① 使用公开密钥加密方式安全的交换在稍后的共享密钥加密中要使用的密钥
 * ② 确保交换的密钥是在安全的前提下，使用共享密钥加密方式进行通信
 * 相互认证 :
 
 * 一个网页完整的HTTPS请求的过程
 1. 客户端输入网址https://www.domain..com，连接到server的443端口。
 2. 服务器返回一个证书（包含公钥、和证书信息，如证书的颁发机构，过期时间等），证书由服务器所拥有的私钥非对称加密生成。
 3. 客户端对证书进行验证（首先会验证证书是否有效，比如颁发机构，过期时间等等）。
 4. 如果客户端验证通过，客户端生成一个随机数，在用服务器返回的证书（公钥）进行加密传输。
 5. 因为公钥是通过服务器的私钥生成，所以服务器是可以对客户端的传回的加密数据进行对称解密的。服务器拿到由客户端生成的随机数，对要传递的数据使用随机数加密。
 6. 客户端收到服务器使用随机数加密的数据进行解密。
 *** 不过在app的开发中因为我们的app通常只需要和一个服务器端进行交互，所以不必要每次请求都从服务器那边获取证书（公钥），在开发中app直接将服务器对应生成的证书（公钥）放在沙盒中，HTTPS请求时只要直接和服务器返回的证书（公钥）进行比对。如果验证通过则使用公钥进行加密在传递回服务器。
     这样即使app中的证书（公钥）被截取，中间人使用证书冒充了服务器与客户端进行通信时（通过了验证），但因为从app返回的数据都是通过证书（公钥）加密的。而中间人从app截取的证书时公钥，缺少对应的私钥即使截获了信息也无法解密。能够最大的程度的保护传递的信息安全。
     PS： 从上面的通信过程中，最重要的是存储在服务器的私钥。因为只有私钥生成了在通信过程中传递的证书（公钥），且只有通过私钥才能对公钥加密的信息进行解密，所以在开发过程中保护好私钥的安全。
 *
 */

#import <Foundation/Foundation.h>
#import <Security/Security.h>

typedef NS_ENUM(NSUInteger, AFSSLPinningMode) {
    AFSSLPinningModeNone, // 代表无条件信任服务器的证书
    AFSSLPinningModePublicKey, // 代表会对服务器返回的证书中的PublicKey进行验证，通过则通过，否则不通过
    AFSSLPinningModeCertificate, //  代表会对服务器返回的证书同本地证书全部进行校验，通过则通过，否则不通过
};

/**
 `AFSecurityPolicy` evaluates server trust against pinned X.509 certificates and public keys over secure connections.

 Adding pinned SSL certificates to your app helps prevent man-in-the-middle attacks and other vulnerabilities. Applications dealing with sensitive customer data or financial information are strongly encouraged to route all communication over an HTTPS connection with SSL pinning configured and enabled.
 */
// *** 说的是AFSecurityPolicy 用来评价通过X.509(数字证书的标准)的数字证书和公开密钥进行的安全网络连接是否值得信任。在应用内添加SSL证书能够有效的防止中间人的攻击和安全漏洞。强烈建议涉及用户敏感或隐私数据或金融信息的应用全部网络连接都采用使用SSL的HTTPS连接。

NS_ASSUME_NONNULL_BEGIN

@interface AFSecurityPolicy : NSObject <NSSecureCoding, NSCopying>

/** 返回SSL Pinning的类型。默认的是AFSSLPinningModeNone。
 The criteria by which server trust should be evaluated against the pinned SSL certificates. Defaults to `AFSSLPinningModeNone`.
 */
@property (readonly, nonatomic, assign) AFSSLPinningMode SSLPinningMode;

/**
 The certificates used to evaluate server trust according to the SSL pinning mode. 

  By default, this property is set to any (`.cer`) certificates included in the target compiling AFNetworking. Note that if you are using AFNetworking as embedded framework, no certificates will be pinned by default. Use `certificatesInBundle` to load certificates from your target, and then create a new policy by calling `policyWithPinningMode:withPinnedCertificates`.
 
 Note that if pinning is enabled, `evaluateServerTrust:forDomain:` will return true if any pinned certificate matches.
 */
// 这个属性保存着所有的可用做校验的证书的集合。AFNetworking默认会搜索工程中所有.cer的证书文件。如果想制定某些证书，可使用certificatesInBundle在目标路径下加载证书，然后调用policyWithPinningMode:withPinnedCertificates创建一个本类对象。
// *** 只要在证书集合中任何一个校验通过，evaluateServerTrust:forDomain: 就会返回true，即通过校验。
@property (nonatomic, strong, nullable) NSSet <NSData *> *pinnedCertificates;

/** 使用允许无效或过期的证书，默认是不允许。
 Whether or not to trust servers with an invalid or expired SSL certificates. Defaults to `NO`.
 */
@property (nonatomic, assign) BOOL allowInvalidCertificates;

/** 是否验证证书中的域名domain
 Whether or not to validate the domain name in the certificate's CN field. Defaults to `YES`.
 */
@property (nonatomic, assign) BOOL validatesDomainName;

///-----------------------------------------
/// @name Getting Certificates from the Bundle
///-----------------------------------------

/**
 Returns any certificates included in the bundle. If you are using AFNetworking as an embedded framework, you must use this method to find the certificates you have included in your app bundle, and use them when creating your security policy by calling `policyWithPinningMode:withPinnedCertificates`.

 @return The certificates included in the given bundle.
 */
// 返回指定bundle中的证书。如果使用AFNetworking的证书验证 ，就必须实现此方法，并且使用policyWithPinningMode:withPinnedCertificates 方法来创建实例对象。
+ (NSSet <NSData *> *)certificatesInBundle:(NSBundle *)bundle;

///-----------------------------------------
/// @name Getting Specific Security Policies
///-----------------------------------------

/**
 Returns the shared default security policy, which does not allow invalid certificates, validates domain name, and does not validate against pinned certificates or public keys.

 @return The default security policy.
 */
/**
 *  默认的实例对象，默认的认证设置为：
 *  1. 不允许无效或过期的证书
 *  2. 验证domain名称
 *  3. 不对证书和公钥进行验证
 */
+ (instancetype)defaultPolicy;

///---------------------
/// @name Initialization 实例化
///---------------------

/**
 Creates and returns a security policy with the specified pinning mode.

 @param pinningMode The SSL pinning mode.

 @return A new security policy.
 */
+ (instancetype)policyWithPinningMode:(AFSSLPinningMode)pinningMode;

/**
 Creates and returns a security policy with the specified pinning mode.

 @param pinningMode The SSL pinning mode.
 @param pinnedCertificates The certificates to pin against.

 @return A new security policy.
 */
+ (instancetype)policyWithPinningMode:(AFSSLPinningMode)pinningMode withPinnedCertificates:(NSSet <NSData *> *)pinnedCertificates;

///------------------------------
/// @name Evaluating Server Trust
///------------------------------

/** 评估服务器信任
 Whether or not the specified server trust should be accepted, based on the security policy.

 This method should be used when responding to an authentication challenge from a server.

 @param serverTrust The X.509 certificate trust of the server.
 @param domain The domain of serverTrust. If `nil`, the domain will not be validated.

 @return Whether or not to trust the server.
 */
- (BOOL)evaluateServerTrust:(SecTrustRef)serverTrust
                  forDomain:(nullable NSString *)domain;

@end

NS_ASSUME_NONNULL_END

///----------------
/// @name Constants
///----------------

/**
 ## SSL Pinning Modes

 The following constants are provided by `AFSSLPinningMode` as possible SSL pinning modes.

 enum {
 AFSSLPinningModeNone,
 AFSSLPinningModePublicKey,
 AFSSLPinningModeCertificate,
 }

 `AFSSLPinningModeNone`
 Do not used pinned certificates to validate servers.

 `AFSSLPinningModePublicKey`
 Validate host certificates against public keys of pinned certificates.

 `AFSSLPinningModeCertificate`
 Validate host certificates against pinned certificates.
*/
