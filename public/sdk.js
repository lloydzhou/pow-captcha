// terser public/sdk.js -m --mangle-props reserved="[render]" > public/sdk.min.js
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

    const render = function(contaner, options = {}) {
        const initializedAttribute = 'data-pow-initialized';
        if (contaner.getAttribute(initializedAttribute) === 'true') {
            console.warn('POW Captcha: This element has already been initialized.');
            return;
        }
        const hiddenInput = document.createElement('input');
        hiddenInput.type = 'hidden';
        hiddenInput.name = 'pow-captcha-token'; // 根据要求设置name
        contaner.appendChild(hiddenInput);    
        const dataset = contaner.dataset;
        if (dataset.timeout) {
            const timeoutValue = parseInt(dataset.timeout, TIMEOUT);
            if (!isNaN(timeoutValue) && timeoutValue > 0) {
                options.timeout = timeoutValue;
            }
        }
        if (dataset.endpoint) {
            options.endpoint = dataset.endpoint;
        }
        contaner.setAttribute(initializedAttribute, 'true');
        pow(options).then(token => {
            hiddenInput.value = token;
            executeCallback(dataset.callback, token);
        }).catch(error => {
            executeCallback(dataset.errorCallback, error);
        });
    }

    function initializeAutoCaptcha() {
        const captchaDivs = document.querySelectorAll('.pow-captcha');
        captchaDivs.forEach(render);
    }
    window.POW['render'] = render; // 允许外部调用
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', initializeAutoCaptcha);
    } else {
        // DOMContentLoaded 事件已触发
        initializeAutoCaptcha();
    }
})()