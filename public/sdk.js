// terser public/sdk.js -m --mangle-props > public/sdk.min.js
(function() {
    const TIMEOUT = 30000; // 30 seconds
    const scripts = document.getElementsByTagName('script');
    const currentScript = scripts[scripts.length - 1];
    const host = new URL(currentScript.src).origin;
    const pow = window.POW = function(options = {}) {
        const endpoint = options.endpoint || host;
        const iframe = document.createElement('iframe');
        let callback = null;
        let timeoutId = null;
        return new Promise((resolve, reject) => {
            callback = function(event) {
                if (event.data.type === 'pow' && event.source === iframe.contentWindow) {
                    resolve(event.data.token);
                }
            }
            timeoutId = setTimeout(() => {
                reject(new Error('POW timeout'));
            }, options.timeout || TIMEOUT);
            window.addEventListener('message', callback);
            iframe.src = endpoint + '/challenge';
            iframe.style.display = 'none';
            document.body.appendChild(iframe);
        }).finally(() => {
            document.body.removeChild(iframe);
            if (callback) {
                window.removeEventListener('message', callback);
            }
            if (timeoutId) {
                clearTimeout(timeoutId);
            }
        });
    };

    function initializeAutoCaptcha() {
        const captchaDivs = document.querySelectorAll('.pow-captcha');
        captchaDivs.forEach(div => {
            const hiddenInput = document.createElement('input');
            hiddenInput.type = 'hidden';
            hiddenInput.name = 'pow-captcha-token'; // 根据要求设置name
            div.appendChild(hiddenInput);

            const powOptions = {};
            if (div.dataset.timeout) {
                const timeoutValue = parseInt(div.dataset.timeout, TIMEOUT);
                if (!isNaN(timeoutValue) && timeoutValue > 0) {
                    powOptions.timeout = timeoutValue;
                }
            }
            // 可以从 data-* 属性中提取其他 POW 选项

            function executeCallback(callbackName, ...args) {
                if (callbackName && typeof window[callbackName] === 'function') {
                    try {
                        return window[callbackName](...args);
                    } catch (e) {
                        console.error('POW Captcha: Error executing callback function "' + callbackName + '".', e);
                    }
                } else if (callbackName) {
                    console.warn('POW Captcha: Callback function "' + callbackName + '" not found or not a function.');
                }
            }

            pow(powOptions)
                .then(token => {
                    hiddenInput.value = token;
                    executeCallback(div.dataset.callback, token);
                })
                .catch(error => {
                    executeCallback(div.dataset.errorCallback, error);
                });
        });
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', initializeAutoCaptcha);
    } else {
        // DOMContentLoaded 事件已触发
        initializeAutoCaptcha();
    }
})()