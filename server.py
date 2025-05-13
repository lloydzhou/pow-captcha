import time
import uuid
import hashlib
import hmac
import secrets
from fastapi import FastAPI, HTTPException
from fastapi.staticfiles import StaticFiles
from fastapi.responses import HTMLResponse
from pydantic import BaseModel
from typing import Optional
import redis

# 创建应用
app = FastAPI()

# Redis客户端
redis_client = redis.Redis(host="localhost", port=6379, db=0)

# 应用密钥
SECRET_KEY = "your_secret_key_change_me"  # 实际应用中应使用环境变量

# 挑战模型
class ChallengeRequest(BaseModel):
    difficulty: int = 4

class ChallengeResponse(BaseModel):
    id: str
    prefix: str
    difficulty: int
    timestamp: int

class VerifyRequest(BaseModel):
    challengeId: str
    nonce: int
    hash: str

class VerifyResponse(BaseModel):
    success: bool
    token: Optional[str] = None
    message: Optional[str] = None

# 工作量证明验证码API
@app.post("/api/pow-challenge", response_model=ChallengeResponse)
async def create_challenge(request: ChallengeRequest):
    # 限制难度范围，防止DoS
    difficulty = max(1, min(6, request.difficulty))
    
    # 创建随机前缀
    challenge_id = str(uuid.uuid4())
    prefix = secrets.token_hex(16)
    timestamp = int(time.time())
    
    # 存储挑战到Redis
    challenge_data = {
        "id": challenge_id,
        "prefix": prefix,
        "difficulty": difficulty,
        "timestamp": timestamp
    }
    
    # 使用管道优化Redis操作
    pipeline = redis_client.pipeline()
    pipeline.hmset(f"pow:challenge:{challenge_id}", challenge_data)
    pipeline.expire(f"pow:challenge:{challenge_id}", 180)  # 3分钟过期
    pipeline.execute()
    
    # 返回挑战
    return ChallengeResponse(
        id=challenge_id,
        prefix=prefix,
        difficulty=difficulty,
        timestamp=timestamp
    )

@app.post("/api/pow-verify", response_model=VerifyResponse)
async def verify_challenge(request: VerifyRequest):
    # 从Redis获取挑战
    challenge_key = f"pow:challenge:{request.challengeId}"
    challenge = redis_client.hgetall(challenge_key)
    
    if not challenge:
        raise HTTPException(status_code=400, detail="Challenge not found or expired")
    
    # 将Redis字节哈希转换为Python字典
    challenge = {k.decode(): v.decode() for k, v in challenge.items()}
    
    # 获取挑战参数
    prefix = challenge["prefix"]
    difficulty = int(challenge["difficulty"])
    timestamp = int(challenge["timestamp"])
    
    # 检查挑战是否过期
    current_time = int(time.time())
    if current_time - timestamp > 180:  # 3分钟过期
        raise HTTPException(status_code=400, detail="Challenge expired")
    
    # 验证哈希
    target = '0' * difficulty
    computed_hash = sha256(f"{prefix}{request.nonce}")
    
    if computed_hash != request.hash or not computed_hash.startswith(target):
        return VerifyResponse(success=False, message="Invalid solution")
    
    # 验证成功，生成令牌
    token = generate_token(request.challengeId, current_time)
    
    # 删除挑战，防止重用
    redis_client.delete(challenge_key)
    
    # 存储已使用的令牌
    token_key = f"pow:token:{token}"
    redis_client.set(token_key, "1")
    redis_client.expire(token_key, 3600)  # 1小时过期
    
    return VerifyResponse(success=True, token=token)

# 验证令牌
@app.post("/api/pow-check-token")
async def check_token(token: str):
    token_key = f"pow:token:{token}"
    exists = redis_client.exists(token_key)
    
    if not exists:
        raise HTTPException(status_code=400, detail="Invalid or expired token")
        
    return {"valid": True}

# SHA-256哈希
def sha256(data):
    return hashlib.sha256(data.encode()).hexdigest()

# 生成加密令牌
def generate_token(challenge_id, timestamp):
    message = f"{challenge_id}:{timestamp}"
    signature = hmac.new(
        SECRET_KEY.encode(),
        message.encode(),
        hashlib.sha256
    ).hexdigest()
    
    return f"{message}:{signature}"


@app.get("/", response_class=HTMLResponse)
async def start_challenge(difficulty: int = 4):
    # 限制难度范围，防止DoS
    difficulty = max(1, min(6, difficulty))
    
    # 创建随机前缀
    challenge_id = str(uuid.uuid4())
    prefix = secrets.token_hex(16)
    timestamp = int(time.time())
    
    # 存储挑战到Redis
    challenge_data = {
        "id": challenge_id,
        "prefix": prefix,
        "difficulty": difficulty,
        "timestamp": timestamp
    }
    
    # 使用管道优化Redis操作
    pipeline = redis_client.pipeline()
    pipeline.hmset(f"pow:challenge:{challenge_id}", challenge_data)
    pipeline.expire(f"pow:challenge:{challenge_id}", 180)  # 3分钟过期
    pipeline.execute()
    
    # 返回挑战
    return HTMLResponse(content=f"""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Proof of Work Challenge</title>
        <script src="/pow.min.js?challege={challenge_id}&prefix={prefix}&difficulty={difficulty}&timestamp={timestamp}" defer ></script>
    </head>
    </html>
    """)


@app.get("/verify", response_class=HTMLResponse)
async def verify(challenge: str, nonce: int, hash: str):
    # 从Redis获取挑战
    challenge_id = challenge
    challenge_key = f"pow:challenge:{challenge_id}"
    challenge = redis_client.hgetall(challenge_key)
    
    if not challenge:
        raise HTTPException(status_code=400, detail="Challenge not found or expired")
    
    # 将Redis字节哈希转换为Python字典
    challenge = {k.decode(): v.decode() for k, v in challenge.items()}
    
    # 获取挑战参数
    prefix = challenge["prefix"]
    difficulty = int(challenge["difficulty"])
    timestamp = int(challenge["timestamp"])
    
    # 检查挑战是否过期
    current_time = int(time.time())
    if current_time - timestamp > 180:  # 3分钟过期
        raise HTTPException(status_code=400, detail="Challenge expired")
    
    # 验证哈希
    target = '0' * difficulty
    computed_hash = sha256(f"{prefix}{nonce}")
    
    if computed_hash != hash or not computed_hash.startswith(target):
        raise HTTPException(status_code=400, detail="Invalid solution")
    
    # 验证成功，生成令牌
    token = generate_token(challenge_id, current_time)
    print('token:', token)
    
    # 删除挑战，防止重用
    redis_client.delete(challenge_key)
    
    # 存储已使用的令牌
    token_key = f"pow:token:{token}"
    redis_client.set(token_key, "1")
    redis_client.expire(token_key, 3600)  # 1小时过期
    
    # return VerifyResponse(success=True, token=token)
    return HTMLResponse(content=f"""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Proof of Work Challenge</title>
        <script src="/pow.min.js?token={token}" defer ></script>
    </head>
    </html>
    """)


app.mount("/", StaticFiles(directory="public"), name="public")