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

在您的 HTML 文件中引入 `sdk.js`，然后调用 `POW()` 函数。它返回一个 Promise，该 Promise 会在成功时解析出 PoW 令牌。

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

在您的服务器端，当用户提交表单或执行受 PoW 验证码保护的操作时，您应该获取接收到的令牌，并通过向您的 PoW 验证码服务的 `/token?token=<TOKEN>` 端点发出请求来验证它。如果服务响应 "VALID"，则令牌有效。
