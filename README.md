# PoW Captcha Service

## English

This project provides a lightweight, efficient, and simple Proof-of-Work (PoW) captcha service. It is built using a custom Redis module, OpenResty, and corresponding JavaScript code.

The Redis module supports the following three commands:
1.  `POW.CHALLENGE <difficulty>`: Creates a new PoW challenge.
2.  `POW.VERIFY <challengeId> <nonce> <hash>`: Verifies a PoW solution.
3.  `POW.TOKEN <token>`: Checks if a verification token is valid.

OpenResty acts as a wrapper around the Redis module, exposing HTTP endpoints for the captcha functionality, while the JavaScript handles the client-side PoW computation.

This setup offers a simple yet effective way to protect your services from bots.

---

## Quick Start

docker run -p 8888:80 lloydzhou/pow-captcha

---

## Usage Example

Include the `sdk.js` in your HTML file and then call the `POW()` function. It returns a Promise that resolves with the PoW token.

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>POWCaptcha Demo</title>
  <!-- Assuming sdk.js is served from the root of your PoW Captcha service -->
  <script src="/sdk.js"></script>
</head>
<body>
  <script>
    // Call the POW function
    // You can pass options like endpoint and timeout
    // e.g., window.POW({ endpoint: 'http://your-pow-captcha-service.com', timeout: 60000 })
    window.POW().then(function(token) {
      console.log('POWCaptcha token:', token);
      // Use the token to verify on your backend
      // For example, make an AJAX request to your server with this token
      // fetch('/your-protected-endpoint', {
      //   method: 'POST',
      //   headers: {
      //     'Content-Type': 'application/json',
      //     'X-POW-TOKEN': token
      //   },
      //   body: JSON.stringify({ /* your data */ })
      // });
      document.write(`<h3>Successfully obtained POWCaptcha token: ${token}</h3>`);
      // For demonstration, this opens a new tab to verify the token directly with the PoW service
      // In a real application, you would send this token to your backend for verification.
      // Your backend would then call the /token endpoint of the PoW service.
      // window.open('/token?token=' + token, '_blank');
    }).catch(function(error) {
      console.error('POWCaptcha error:', error);
      document.write(`<h3>Failed to obtain POWCaptcha token: ${error.message}</h3>`);
    });
  </script>
</body>
</html>
```

The `POW()` function accepts an optional `options` object:
-   `endpoint`: (Optional) The base URL of your PoW Captcha service. Defaults to the origin of `sdk.js`.
-   `timeout`: (Optional) Timeout in milliseconds for the PoW challenge. Defaults to 30000 (30 seconds).

On your server-side, when a user submits a form or performs an action protected by PoW Captcha, you should take the received token and verify it by making a request to the `/token?token=<TOKEN>` endpoint of your PoW Captcha service. If the service responds with "VALID", the token is good.