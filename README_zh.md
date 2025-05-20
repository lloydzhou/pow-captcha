# PoW 验证码服务

## 中文 (Chinese)

本项目提供了一个轻量、高效、简洁的工作量证明 (PoW) 验证码服务。它通过自定义的 Redis 模块、OpenResty 以及配套的 JavaScript 代码实现。

Redis 模块支持以下三个命令：
1.  `POW.CHALLENGE <difficulty>`: 创建一个新的 PoW 挑战。
2.  `POW.VERIFY <challengeId> <nonce> <hash>`: 验证 PoW 解决方案。
3.  `POW.TOKEN <token>`: 检查验证令牌是否有效。

OpenResty 作为 Redis 模块的简单封装，提供了用于验证码功能的 HTTP 接口，而 JavaScript 负责客户端的工作量证明计算。

该服务为您提供了一种简单而有效的方式来保护您的服务免受机器人侵害。

---

## 快速开始

docker run -p 8888:80 lloydzhou/pow-captcha

---

## 使用示例

有多种方式集成 PoW 验证码：

**1. 自动渲染 (推荐)**

最简单的方式是在您的 HTML 中声明一个 `div` 元素，并赋予其 `pow-captcha` 类。SDK 会自动找到这些 `div`，在其中渲染一个隐藏的 `input` 字段，并填充 PoW 令牌。

```html
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>POWCaptcha 自动渲染演示</title>
  <script src="/sdk.js"></script>
  <script>
    // 可选的回调函数
    function myCaptchaSuccessCallback(token) {
      console.log('验证码成功，令牌:', token);
      // 您可以在这里处理令牌，例如启用提交按钮
      document.getElementById('submit-button').disabled = false;
    }
    function myCaptchaErrorCallback(error) {
      console.error('验证码失败:', error);
      // 您可以在这里处理错误
    }
  </script>
</head>
<body>
  <form id="myForm" action="/submit-data" method="post">
    <!-- 其他表单字段 -->
    <label for="username">用户名:</label>
    <input type="text" id="username" name="username">
    <br>

    <!-- PoW 验证码容器 -->
    <!-- SDK 将在此 div 内自动创建一个名为 "pow-captcha-token" 的隐藏 input -->
    <div class="pow-captcha"
         data-callback="myCaptchaSuccessCallback"
         data-error-callback="myCaptchaErrorCallback"
         data-timeout="45000">
      <!-- 您可以在这里放置加载指示器或提示信息 -->
      正在加载验证码...
    </div>
    <br>

    <button type="submit" id="submit-button" disabled>提交</button>
  </form>

  <script>
    // 表单提交时，隐藏的 input (pow-captcha-token) 会随表单一起提交
    // document.getElementById('myForm').addEventListener('submit', function(event) {
    //   const tokenInput = this.querySelector('input[name="pow-captcha-token"]');
    //   if (!tokenInput || !tokenInput.value) {
    //     event.preventDefault(); // 如果没有令牌，阻止提交 (可选)
    //     alert('请等待验证码完成。');
    //   }
    // });
  </script>
</body>
</html>
```

`pow-captcha` `div` 支持以下 `data-*` 属性：
-   `data-callback`: (可选) PoW 成功时调用的全局 JavaScript 函数名。该函数将接收令牌作为其第一个参数。
-   `data-error-callback`: (可选) PoW 失败时调用的全局 JavaScript 函数名。该函数将接收错误对象作为其第一个参数。
-   `data-timeout`: (可选) PoW 挑战的超时时间（毫秒）。默认为 30000 (30 秒)。
-   `data-endpoint`: (可选) 您的 PoW 验证码服务的基础 URL。默认为 `sdk.js` 脚本所在的域。

**2. 手动渲染**

如果您需要更细致的控制，可以调用 `window.POW.render(containerElement, options)` 函数。

```html
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <title>POWCaptcha 手动渲染演示</title>
  <script src="/sdk.js"></script>
  <script>
    function myCallback(token) {
      console.log('手动渲染成功，令牌:', token);
      document.getElementById('token-display').innerText = '获取到的令牌: ' + token;
    }
    function myErrorCallback(error) {
      console.error('手动渲染失败:', error);
      document.getElementById('token-display').innerText = '获取令牌失败: ' + error.message;
    }

    // DOM 加载完成后执行
    document.addEventListener('DOMContentLoaded', function() {
      const captchaContainer = document.getElementById('my-captcha-container');
      if (captchaContainer) {
        window.POW.render(captchaContainer, {
          // options 对象与 POW() 函数的类似
          // timeout: 45000, // 毫秒
          // endpoint: 'http://your-pow-service.com'
          // 注意：如果容器元素上有 data-callback, data-error-callback, data-timeout, data-endpoint
          // 它们会覆盖这里传递的 options，除非 options 中未定义。
          // 为了清晰，建议将回调通过 data-* 属性定义或仅通过 options 定义。
        });
      }
    });
  </script>
</head>
<body>
  <div id="my-captcha-container" data-callback="myCallback" data-error-callback="myErrorCallback">
    <!-- 手动渲染将在此处进行 -->
  </div>
  <p id="token-display"></p>
</body>
</html>
```
`window.POW.render` 函数的第二个参数 `options` 对象与下面描述的 `window.POW()` 函数的 `options` 对象类似。此外，容器元素上的 `data-*` 属性（如 `data-callback`, `data-timeout`）也会被考虑，并且如果它们存在，通常会优先于 `options` 对象中的相应设置。

**3. 直接调用 `POW()` 函数**

您也可以直接调用 `window.POW()` 函数。它返回一个 Promise，该 Promise 会在成功时解析出 PoW 令牌。

```html
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>POWCaptcha 演示</title>
  <!-- 假设 sdk.js 从您的 PoW 验证码服务的根目录提供 -->
  <script src="/sdk.js"></script>
</head>
<body>
  <script>
    // 调用 POW 函数
    // 您可以传递诸如 endpoint 和 timeout 之类的选项
    // 例如：window.POW({ endpoint: 'http://your-pow-captcha-service.com', timeout: 60000 })
    window.POW().then(function(token) {
      console.log('POWCaptcha 令牌:', token);
      // 使用此令牌在您的后端进行验证
      // 例如，向您的服务器发送一个带有此令牌的 AJAX 请求
      // fetch('/your-protected-endpoint', {
      //   method: 'POST',
      //   headers: {
      //     'Content-Type': 'application/json',
      //     'X-POW-TOKEN': token // 通常通过请求头或请求体发送
      //   },
      //   body: JSON.stringify({ /* 您的数据 */ })
      // });
      document.write(`<h3>成功获取 POWCaptcha 令牌: ${token}</h3>`);
      // 作为演示，这里会打开一个新标签页直接使用 PoW 服务验证令牌
      // 在实际应用中，您应该将此令牌发送到您的后端进行验证。
      // 您的后端随后会调用 PoW 服务的 /token 接口。
      // window.open('/token?token=' + token, '_blank');
    }).catch(function(error) {
      console.error('POWCaptcha 错误:', error);
      document.write(`<h3>获取 POWCaptcha 令牌失败: ${error.message}</h3>`);
    });
  </script>
</body>
</html>
```

`POW()` 函数接受一个可选的 `options` 对象：
-   `endpoint`: (可选) 您的 PoW 验证码服务的基础 URL。默认为 `sdk.js` 脚本所在的域。
-   `timeout`: (可选) PoW 挑战的超时时间（毫秒）。默认为 30000 (30 秒)。

在您的服务器端，当用户提交表单或执行受 PoW 验证码保护的操作时，您应该获取接收到的令牌（对于自动或手动渲染，它通常在名为 `pow-captcha-token` 的隐藏 input 中），并通过向您的 PoW 验证码服务的 `/token?token=<TOKEN>` 端点发出请求来验证它。如果服务响应 "VALID"，则令牌有效。
