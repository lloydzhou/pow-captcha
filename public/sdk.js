(function() {
    const scripts = document.getElementsByTagName('script');
    const currentScript = scripts[scripts.length - 1];
    const host = new URL(currentScript.src).origin;
    window.POW = function(options = {}) {
        const endpoint = options.endpoint || host;
        const iframe = document.createElement('iframe');
        let callback = null;
        let timeoutId = null;
        return new Promise((resolve, reject) => {
            callback = function(event) {
                if (event.data.type === 'pow') {
                    resolve(event.data.token);
                }
            }
            timeoutId = setTimeout(() => {
                reject(new Error('POW timeout'));
            }, options.timeout || 30000);
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
    }
})()