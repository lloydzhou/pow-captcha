/**
 * PoW-Captcha - 基于计算工作量证明的验证码
 */
(function() {
  const POWCaptcha = {
    // 初始化验证码
    init: function(options = {}) {
      this.options = Object.assign({
        selector: '#pow-captcha',
        difficulty: 4,  // 难度系数 (4-6效果较好)
        timeout: 30000, // 超时时间
        onSuccess: null,
        onError: null,
        onProgress: null
      }, options);

      this.challenge = null;
      this.worker = null;
      this.status = 'idle';

      this.container = document.querySelector(this.options.selector);
      if (!this.container) {
        console.error(`Container ${this.options.selector} not found`);
        return;
      }else {
        this.render();
      }
      return this;
    },
    
    // 渲染界面
    render: function() {
      this.container.innerHTML = `
        <div class="pow-captcha-wrapper">
          <div class="pow-captcha-challenge">
            <div class="pow-captcha-status">点击验证</div>
            <div class="pow-captcha-progress-bar">
              <div class="pow-captcha-progress"></div>
            </div>
            <button class="pow-captcha-button">验证</button>
          </div>
          <input type="hidden" name="pow-captcha-token" value="">
        </div>
      `;
      
      // 添加CSS
      if (!document.getElementById('pow-captcha-styles')) {
        const style = document.createElement('style');
        style.id = 'pow-captcha-styles';
        style.textContent = `
          .pow-captcha-wrapper {
            border: 1px solid #ddd;
            border-radius: 4px;
            padding: 15px;
            background: #f9f9f9;
            width: 300px;
            box-sizing: border-box;
          }
          .pow-captcha-challenge {
            display: flex;
            flex-direction: column;
            align-items: center;
          }
          .pow-captcha-status {
            margin-bottom: 10px;
            font-size: 14px;
            color: #555;
          }
          .pow-captcha-progress-bar {
            width: 100%;
            height: 6px;
            background: #eee;
            border-radius: 3px;
            margin-bottom: 15px;
            overflow: hidden;
            display: none;
          }
          .pow-captcha-progress {
            height: 100%;
            width: 0%;
            background: #4CAF50;
            transition: width 0.3s;
          }
          .pow-captcha-button {
            background: #2196F3;
            border: none;
            color: white;
            padding: 8px 16px;
            border-radius: 4px;
            cursor: pointer;
            font-size: 14px;
          }
          .pow-captcha-button:hover {
            background: #0b7dda;
          }
          .pow-captcha-button:disabled {
            background: #cccccc;
            cursor: not-allowed;
          }
        `;
        document.head.appendChild(style);
      }
      
      // 添加事件处理
      this.button = this.container.querySelector('.pow-captcha-button');
      this.statusContainer = this.container.querySelector('.pow-captcha-status');
      this.progress = this.container.querySelector('.pow-captcha-progress');
      this.progressBar = this.container.querySelector('.pow-captcha-progress-bar');
      this.token = this.container.querySelector('input[name="pow-captcha-token"]');
      
      this.button.addEventListener('click', () => this.startVerification());
    },
    
    // 开始验证过程
    async startVerification() {
      if (this.status === 'running') return;
      
      this.status = 'running';
      this.button.disabled = true;
      this.statusContainer.textContent = '正在请求挑战...';
      
      try {
        // 请求挑战
        const challenge = await this.requestChallenge();
        this.challenge = challenge;
        
        // 显示进度条
        this.progressBar.style.display = 'block';
        this.statusContainer.textContent = '计算中...';
        
        // 启动工作线程解决挑战
        this.solveChallenge(challenge);
        
      } catch (error) {
        console.error('Verification error:', error);
        this.statusContainer.textContent = '验证失败，请重试';
        this.button.disabled = false;
        this.status = 'idle';
        
        if (this.options.onError) {
          this.options.onError(error);
        }
      }
    },
    
    // 请求挑战
    async requestChallenge() {
      const response = await fetch('/api/pow-challenge', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({
          difficulty: this.options.difficulty
        })
      });
      
      if (!response.ok) {
        throw new Error('Failed to get challenge');
      }
      
      return await response.json();
    },
    
    // 解决挑战 (在Web Worker中运行以避免阻塞UI)
    solveChallenge(challenge) {
      // 创建Worker Blob URL        
      const workerScript = `
        // SHA-256实现
        function sha256(b){function h(j,k){return(j>>e)+(k>>e)+((p=(j&o)+(k&o))>>e)<<e|p&o}function f(j,k){return j>>>k|j<<32-k}var g=[],d,c=3,l=[2],p,i,q,a,m=[],n=[];i=b.length*8;for(var e=16,o=65535,r="";c<312;c++){for(d=l.length;d--&&c%l[d]!=0;);d<0&&l.push(c)}b+="\u0080";for(c=0;c<=i;c+=8)n[c>>5]|=(b.charCodeAt(c/8)&255)<<24-c%32;n[(i+64>>9<<4)+15]=i;for(c=8;c--;)m[c]=parseInt(Math.pow(l[c],0.5).toString(e).substr(2,8),e);for(c=0;c<n.length;c+=e){a=m.slice(0);for(b=0;b<64;b++){g[b]=b<e?n[b+c]:h(h(h(f(g[b-2],17)^f(g[b-2],19)^g[b-2]>>>10,g[b-7]),f(g[b-15],7)^f(g[b-15],18)^g[b-15]>>>3),g[b-e]);i=h(h(h(h(a[7],f(a[4],6)^f(a[4],11)^f(a[4],25)),a[4]&a[5]^~a[4]&a[6]),parseInt(Math.pow(l[b],1/3).toString(e).substr(2,8),e)),g[b]);q=(f(a[0],2)^f(a[0],13)^f(a[0],22))+(a[0]&a[1]^a[0]&a[2]^a[1]&a[2]);for(d=8;--d;)a[d]=d==4?h(a[3],i):a[d-1];a[0]=h(i,q)}for(d=8;d--;)m[d]+=a[d]}for(c=0;c<8;c++)for(b=8;b--;)r+=(m[c]>>>b*4&15).toString(e);return r}

        // 解决工作量证明挑战
        function solvePOW(prefix, difficulty, maxAttempts) {
          const target = '0'.repeat(difficulty);
          let nonce = 0;
          let hash = '';
          
          while (nonce < maxAttempts) {
            // 计算当前nonce的哈希
            hash = sha256(prefix + nonce);
            
            // 检查哈希前缀
            if (hash.startsWith(target)) {
              return { success: true, nonce, hash };
            }
            
            // 每100次计算报告进度
            if (nonce % 100 === 0) {
              self.postMessage({ type: 'progress', nonce });
            }
            
            nonce++;
          }
          
          return { success: false, message: '达到最大尝试次数' };
        }
        
        // 监听主线程消息
        self.onmessage = function(e) {
          const { prefix, difficulty, maxAttempts } = e.data;
          const result = solvePOW(prefix, difficulty, maxAttempts);
          self.postMessage({ type: 'result', ...result });
        };
      `;
      
      const blob = new Blob([workerScript], { type: 'application/javascript' });
      const workerUrl = URL.createObjectURL(blob);
      
      // 创建和启动Worker
      if (this.worker) {
        this.worker.terminate();
      }
      
      this.worker = new Worker(workerUrl);
      this.worker.onmessage = (e) => this.handleWorkerMessage(e.data);
      
      // 设置超时
      this.timeout = setTimeout(() => {
        this.worker.terminate();
        this.statusContainer.textContent = '验证超时，请重试';
        this.button.disabled = false;
        this.status = 'idle';
        this.progress.style.width = '0%';
        this.progressBar.style.display = 'none';
      }, this.options.timeout);
      
      // 发送挑战给Worker
      this.worker.postMessage({
        prefix: challenge.prefix,
        difficulty: challenge.difficulty,
        maxAttempts: 10000000 // 最大尝试次数
      });
    },
    
    // 处理Worker消息
    async handleWorkerMessage(data) {
      if (data.type === 'progress') {
        // 更新进度
        const progress = Math.min((data.nonce / 100000) * 100, 95);
        this.progress.style.width = progress + '%';
        
        if (this.options.onProgress) {
          this.options.onProgress(progress);
        }
      } else if (data.type === 'result') {
        clearTimeout(this.timeout);
        
        if (data.success) {
          this.progress.style.width = '100%';
          this.statusContainer.textContent = '验证中...';
          
          try {
            // 将结果发送给服务器验证
            const verified = await this.verifyChallenge(data.nonce, data.hash);
            
            if (verified.success) {
              this.statusContainer.textContent = '验证成功';
              this.token.value = verified.token;

              if (this.options.onSuccess) {
                this.options.onSuccess(verified.token);
              }
            } else {
              this.statusContainer.textContent = '验证失败，请重试';
            }
          } catch (error) {
            console.error('Verification error:', error);
            this.statusContainer.textContent = '验证出错，请重试';
            
            if (this.options.onError) {
              this.options.onError(error);
            }
          }
        } else {
          this.statusContainer.textContent = '计算失败，请重试';
          
          if (this.options.onError) {
            this.options.onError(new Error(data.message));
          }
        }
        
        // 重置状态
        setTimeout(() => {
          this.button.disabled = false;
          this.status = 'idle';
          this.worker.terminate();
          this.worker = null;
        }, 1000);
      }
    },
    
    // 服务器验证结果
    async verifyChallenge(nonce, hash) {
      const response = await fetch('/api/pow-verify', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({
          challengeId: this.challenge.id,
          nonce,
          hash
        })
      });
      
      if (!response.ok) {
        throw new Error('Failed to verify challenge');
      }
      
      return await response.json();
    },
    
    // 获取令牌
    getToken: function() {
      return this.token ? this.token.value : '';
    },
    
    // 重置验证码
    reset: function() {
      if (this.worker) {
        this.worker.terminate();
        this.worker = null;
      }
      
      clearTimeout(this.timeout);
      this.statusContainerstatusContainer.textContent = '点击验证';
      this.button.disabled = false;
      this.progress.style.width = '0%';
      this.progressBar.style.display = 'none';
      this.token.value = '';
      this.status = 'idle';
      this.challenge = null;
    }
  };
  
  // 导出到全局
  window.POWCaptcha = POWCaptcha;
})();