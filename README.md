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

There are multiple ways to integrate the PoW Captcha:

**1. Automatic Rendering (Recommended)**

The easiest way is to declare a `div` element in your HTML with the class `pow-captcha`. The SDK will automatically find these divs, render a hidden `input` field within them, and populate it with the PoW token.

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>POWCaptcha Auto Render Demo</title>
  <script src="/sdk.js"></script>
  <script>
    // Optional callback functions
    function myCaptchaSuccessCallback(token) {
      console.log('Captcha success, token:', token);
      // You can handle the token here, e.g., enable a submit button
      document.getElementById('submit-button').disabled = false;
    }
    function myCaptchaErrorCallback(error) {
      console.error('Captcha failed:', error);
      // You can handle the error here
    }
  </script>
</head>
<body>
  <form id="myForm" action="/submit-data" method="post">
    <!-- Other form fields -->
    <label for="username">Username:</label>
    <input type="text" id="username" name="username">
    <br>

    <!-- PoW Captcha container -->
    <!-- The SDK will automatically create a hidden input named "pow-captcha-token" inside this div -->
    <div class="pow-captcha"
         data-callback="myCaptchaSuccessCallback"
         data-error-callback="myCaptchaErrorCallback"
         data-timeout="45000">
      <!-- You can place a loading indicator or message here -->
      Loading captcha...
    </div>
    <br>

    <button type="submit" id="submit-button" disabled>Submit</button>
  </form>

  <script>
    // When the form is submitted, the hidden input (pow-captcha-token) will be submitted with it
    // document.getElementById('myForm').addEventListener('submit', function(event) {
    //   const tokenInput = this.querySelector('input[name="pow-captcha-token"]');
    //   if (!tokenInput || !tokenInput.value) {
    //     event.preventDefault(); // Prevent submission if no token (optional)
    //     alert('Please wait for the captcha to complete.');
    //   }
    // });
  </script>
</body>
</html>
```

The `pow-captcha` `div` supports the following `data-*` attributes:
-   `data-callback`: (Optional) The name of a global JavaScript function to call upon successful PoW. The token will be passed as its first argument.
-   `data-error-callback`: (Optional) The name of a global JavaScript function to call upon PoW failure. The error object will be passed as its first argument.
-   `data-timeout`: (Optional) Timeout in milliseconds for the PoW challenge. Defaults to 30000 (30 seconds).
-   `data-endpoint`: (Optional) The base URL of your PoW Captcha service. Defaults to the origin of `sdk.js`.

**2. Manual Rendering**

If you need more fine-grained control, you can call the `window.POW.render(containerElement, options)` function.

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>POWCaptcha Manual Render Demo</title>
  <script src="/sdk.js"></script>
  <script>
    function myCallback(token) {
      console.log('Manual render success, token:', token);
      document.getElementById('token-display').innerText = 'Token obtained: ' + token;
    }
    function myErrorCallback(error) {
      console.error('Manual render failed:', error);
      document.getElementById('token-display').innerText = 'Failed to obtain token: ' + error.message;
    }

    // Execute after DOM is loaded
    document.addEventListener('DOMContentLoaded', function() {
      const captchaContainer = document.getElementById('my-captcha-container');
      if (captchaContainer) {
        window.POW.render(captchaContainer, {
          // Options object is similar to the POW() function's
          // timeout: 45000, // milliseconds
          // endpoint: 'http://your-pow-service.com'
          // Note: If data-callback, data-error-callback, data-timeout, data-endpoint are present on the container element,
          // they will override options passed here unless not defined in options.
          // For clarity, define callbacks either via data-* attributes or solely via options.
        });
      }
    });
  </script>
</head>
<body>
  <div id="my-captcha-container" data-callback="myCallback" data-error-callback="myErrorCallback">
    <!-- Manual rendering will happen here -->
  </div>
  <p id="token-display"></p>
</body>
</html>
```
The second argument to `window.POW.render`, the `options` object, is similar to the `options` object for the `window.POW()` function described below. Additionally, `data-*` attributes on the container element (like `data-callback`, `data-timeout`) are also considered and will generally take precedence over corresponding settings in the `options` object if present.

**3. Calling `POW()` Directly**

You can also call the `window.POW()` function directly. It returns a Promise that resolves with the PoW token upon success.

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

On your server-side, when a user submits a form or performs an action protected by PoW Captcha, you should take the received token (for automatic or manual rendering, it's typically in a hidden input named `pow-captcha-token`) and verify it by making a request to the `/token?token=<TOKEN>` endpoint of your PoW Captcha service. If the service responds with "VALID", the token is good.