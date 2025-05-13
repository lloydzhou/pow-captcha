#include "redismodule.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <uuid/uuid.h>

// 生成UUID字符串
static void generateUUID(char *uuidStr) {
    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse(uuid, uuidStr);
}

// 生成随机十六进制字符串作为前缀
static void generateRandomHex(char *hex, size_t length) {
    static const char hexchars[] = "0123456789abcdef";
    for (size_t i = 0; i < length; i++) {
        hex[i] = hexchars[rand() % 16];
    }
    hex[length] = '\0';
}

// SHA-256哈希函数
static void sha256(const char *input, char *output) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input, strlen(input));
    SHA256_Final(hash, &sha256);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[SHA256_DIGEST_LENGTH * 2] = '\0';
}

// 检查哈希是否满足难度要求
static int checkHashDifficulty(const char *hash, int difficulty) {
    for (int i = 0; i < difficulty; i++) {
        if (hash[i] != '0')
            return 0;
    }
    return 1;
}

// POW.CHALLENGE <difficulty> - 创建工作量证明挑战
int POWChallenge_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 2) {
        return RedisModule_WrongArity(ctx);
    }
    
    long long difficulty;
    if (RedisModule_StringToLongLong(argv[1], &difficulty) != REDISMODULE_OK) {
        return RedisModule_ReplyWithError(ctx, "ERR invalid difficulty value");
    }
    
    // 限制难度范围
    if (difficulty < 1) difficulty = 1;
    if (difficulty > 6) difficulty = 6;
    
    // 创建挑战ID和前缀
    RedisModule_Log(ctx, "notice", "POWChallenge: Creating challenge with difficulty %lld", difficulty);
    
    char challenge_id[37];
    generateUUID(challenge_id);
    
    char prefix[33];
    generateRandomHex(prefix, 32);
    
    time_t timestamp = time(NULL);
    
    // 采用更安全的方式存储挑战信息 - 使用Redis命令而不是直接使用API
    RedisModuleCallReply *reply;
    
    // 首先检查键是否已存在
    RedisModuleString *key_name = RedisModule_CreateStringPrintf(ctx, "pow:challenge:%s", challenge_id);
    if (!key_name) {
        return RedisModule_ReplyWithError(ctx, "ERR failed to create key name");
    }
    
    reply = RedisModule_Call(ctx, "EXISTS", "s", key_name);
    if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_INTEGER && 
        RedisModule_CallReplyInteger(reply) > 0) {
        RedisModule_FreeCallReply(reply);
        RedisModule_FreeString(ctx, key_name);
        return RedisModule_ReplyWithError(ctx, "ERR challenge ID already exists");
    }
    RedisModule_FreeCallReply(reply);
    
    // 使用HSET命令单独设置每个字段
    RedisModuleString *id_str = RedisModule_CreateString(ctx, challenge_id, strlen(challenge_id));
    RedisModuleString *prefix_str = RedisModule_CreateString(ctx, prefix, strlen(prefix));
    RedisModuleString *difficulty_str = RedisModule_CreateStringFromLongLong(ctx, difficulty);
    RedisModuleString *timestamp_str = RedisModule_CreateStringFromLongLong(ctx, timestamp);
    
    if (!id_str || !prefix_str || !difficulty_str || !timestamp_str) {
        if (id_str) RedisModule_FreeString(ctx, id_str);
        if (prefix_str) RedisModule_FreeString(ctx, prefix_str);
        if (difficulty_str) RedisModule_FreeString(ctx, difficulty_str);
        if (timestamp_str) RedisModule_FreeString(ctx, timestamp_str);
        RedisModule_FreeString(ctx, key_name);
        return RedisModule_ReplyWithError(ctx, "ERR failed to create hash field values");
    }
    
    // 单独设置每个字段，而不是一次性设置所有字段
    reply = RedisModule_Call(ctx, "HSET", "sssssssss", 
        key_name, 
        RedisModule_CreateString(ctx, "id", 2), id_str,
        RedisModule_CreateString(ctx, "prefix", 6), prefix_str,
        RedisModule_CreateString(ctx, "difficulty", 10), difficulty_str,
        RedisModule_CreateString(ctx, "timestamp", 9), timestamp_str
    );
    
    if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_ERROR) {
        RedisModule_Log(ctx, "warning", "POWChallenge: HSET failed: %s",
            RedisModule_CallReplyStringPtr(reply, NULL));
        RedisModule_FreeCallReply(reply);
        RedisModule_FreeString(ctx, key_name);
        RedisModule_FreeString(ctx, id_str);
        RedisModule_FreeString(ctx, prefix_str);
        RedisModule_FreeString(ctx, difficulty_str);
        RedisModule_FreeString(ctx, timestamp_str);
        return RedisModule_ReplyWithError(ctx, "ERR failed to set challenge data");
    }
    RedisModule_FreeCallReply(reply);
    
    // 设置过期时间 - 修复为正确的"sl"格式
    reply = RedisModule_Call(ctx, "EXPIRE", "sl", key_name, 180);
    
    if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_ERROR) {
        RedisModule_Log(ctx, "warning", "POWChallenge: EXPIRE failed");
        RedisModule_FreeCallReply(reply);
        // 尝试删除已创建的键
        RedisModule_Call(ctx, "DEL", "s", key_name);
        RedisModule_FreeString(ctx, key_name);
        RedisModule_FreeString(ctx, id_str);
        RedisModule_FreeString(ctx, prefix_str);
        RedisModule_FreeString(ctx, difficulty_str);
        RedisModule_FreeString(ctx, timestamp_str);
        return RedisModule_ReplyWithError(ctx, "ERR failed to set expiry");
    }
    RedisModule_FreeCallReply(reply);
    
    // 释放不再需要的字符串
    RedisModule_FreeString(ctx, key_name);
    RedisModule_FreeString(ctx, id_str);
    RedisModule_FreeString(ctx, prefix_str);
    RedisModule_FreeString(ctx, difficulty_str);
    RedisModule_FreeString(ctx, timestamp_str);
    
    // 返回挑战信息
    RedisModule_ReplyWithArray(ctx, 4);
    RedisModule_ReplyWithStringBuffer(ctx, challenge_id, strlen(challenge_id));
    RedisModule_ReplyWithStringBuffer(ctx, prefix, strlen(prefix));
    RedisModule_ReplyWithLongLong(ctx, difficulty);
    RedisModule_ReplyWithLongLong(ctx, timestamp);
    
    RedisModule_Log(ctx, "notice", "POWChallenge: Challenge created successfully: %s", challenge_id);
    return REDISMODULE_OK;
}

// POW.VERIFY <challengeId> <nonce> <hash> - 验证工作量证明挑战
int POWVerify_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 4) {
        return RedisModule_WrongArity(ctx);
    }
    
    size_t id_len, nonce_len, hash_len;
    const char *challenge_id = RedisModule_StringPtrLen(argv[1], &id_len);
    const char *nonce_str = RedisModule_StringPtrLen(argv[2], &nonce_len);
    const char *hash = RedisModule_StringPtrLen(argv[3], &hash_len);
    
    // 检查输入参数是否有效
    if (!challenge_id || !nonce_str || !hash || id_len == 0 || nonce_len == 0 || hash_len == 0) {
        return RedisModule_ReplyWithError(ctx, "ERR invalid input parameters");
    }
    
    RedisModule_Log(ctx, "notice", "POWVerify: Verifying challenge %s", challenge_id);
    
    // 创建挑战键名
    RedisModuleString *key_name = RedisModule_CreateStringPrintf(ctx, "pow:challenge:%s", challenge_id);
    if (!key_name) {
        return RedisModule_ReplyWithError(ctx, "ERR failed to create key name");
    }
    
    // 首先检查键是否存在
    RedisModuleCallReply *reply = RedisModule_Call(ctx, "EXISTS", "s", key_name);
    if (RedisModule_CallReplyType(reply) != REDISMODULE_REPLY_INTEGER || 
        RedisModule_CallReplyInteger(reply) != 1) {
        RedisModule_FreeCallReply(reply);
        RedisModule_FreeString(ctx, key_name);
        return RedisModule_ReplyWithError(ctx, "ERR challenge not found or expired");
    }
    RedisModule_FreeCallReply(reply);

    RedisModule_Log(ctx, "notice", "POWVerify: Challenge %s found", challenge_id);

    RedisModule_Log(ctx, "notice", "POWVerify: Challenge data retrieved successfully");
    RedisModuleCallReply *prefix_reply = RedisModule_Call(ctx, "HGET", "sc", key_name, "prefix");
    RedisModuleCallReply *difficulty_reply = RedisModule_Call(ctx, "HGET", "sc", key_name, "difficulty");
    RedisModuleCallReply *timestamp_reply = RedisModule_Call(ctx, "HGET", "sc", key_name, "timestamp");
    
    // 检查所有字段是否正确返回
    if (RedisModule_CallReplyType(prefix_reply) != REDISMODULE_REPLY_STRING || 
        RedisModule_CallReplyType(difficulty_reply) != REDISMODULE_REPLY_STRING ||
        RedisModule_CallReplyType(timestamp_reply) != REDISMODULE_REPLY_STRING) {
        
        if (prefix_reply) RedisModule_FreeCallReply(prefix_reply);
        if (difficulty_reply) RedisModule_FreeCallReply(difficulty_reply);
        if (timestamp_reply) RedisModule_FreeCallReply(timestamp_reply);
        RedisModule_FreeString(ctx, key_name);
        return RedisModule_ReplyWithError(ctx, "ERR invalid challenge data");
    }

    RedisModule_Log(ctx, "notice", "POWVerify: Challenge data retrieved successfully");
    
    // 获取字段值
    size_t prefix_len;
    const char *prefix = RedisModule_CallReplyStringPtr(prefix_reply, &prefix_len);
    
    long long difficulty = 0, timestamp = 0;
    RedisModuleString *difficulty_str = RedisModule_CreateStringFromCallReply(difficulty_reply);
    RedisModuleString *timestamp_str = RedisModule_CreateStringFromCallReply(timestamp_reply);
    
    if (RedisModule_StringToLongLong(difficulty_str, &difficulty) != REDISMODULE_OK ||
        RedisModule_StringToLongLong(timestamp_str, &timestamp) != REDISMODULE_OK) {
        
        RedisModule_FreeCallReply(prefix_reply);
        RedisModule_FreeCallReply(difficulty_reply);
        RedisModule_FreeCallReply(timestamp_reply);
        RedisModule_FreeString(ctx, key_name);
        RedisModule_FreeString(ctx, difficulty_str);
        RedisModule_FreeString(ctx, timestamp_str);
        return RedisModule_ReplyWithError(ctx, "ERR invalid numeric values");
    }

    RedisModule_Log(ctx, "notice", "POWVerify: Parsed difficulty: %lld, timestamp: %lld", difficulty, timestamp);
    
    // 检查挑战是否过期
    time_t current_time = time(NULL);
    if (current_time - timestamp > 180) {
        RedisModule_FreeCallReply(prefix_reply);
        RedisModule_FreeCallReply(difficulty_reply);
        RedisModule_FreeCallReply(timestamp_reply);
        RedisModule_FreeString(ctx, key_name);
        RedisModule_FreeString(ctx, difficulty_str);
        RedisModule_FreeString(ctx, timestamp_str);
        return RedisModule_ReplyWithError(ctx, "ERR challenge expired");
    }
    
    // 验证哈希 - 首先检查缓冲区大小安全性
    if (prefix_len + nonce_len >= 255) {
        RedisModule_FreeCallReply(prefix_reply);
        RedisModule_FreeCallReply(difficulty_reply);
        RedisModule_FreeCallReply(timestamp_reply);
        RedisModule_FreeString(ctx, key_name);
        RedisModule_FreeString(ctx, difficulty_str);
        RedisModule_FreeString(ctx, timestamp_str);
        return RedisModule_ReplyWithError(ctx, "ERR input too long");
    }
    
    // 结合前缀和nonce
    char combined[256];
    snprintf(combined, sizeof(combined), "%s%s", prefix, nonce_str);
    
    // 计算哈希
    char computed_hash[65];
    sha256(combined, computed_hash);
    
    // 释放不再需要的资源
    RedisModule_FreeCallReply(prefix_reply);
    RedisModule_FreeCallReply(difficulty_reply);
    RedisModule_FreeCallReply(timestamp_reply);
    RedisModule_FreeString(ctx, difficulty_str);
    RedisModule_FreeString(ctx, timestamp_str);
    
    // 验证哈希
    if (strcmp(computed_hash, hash) != 0 || !checkHashDifficulty(computed_hash, difficulty)) {
        RedisModule_FreeString(ctx, key_name);
        RedisModule_ReplyWithSimpleString(ctx, "FAIL");
        return REDISMODULE_OK;
    }
    
    char token[37];
    generateUUID(token);
    RedisModule_Log(ctx, "notice", "POWVerify: Token generated: %s", token);    
    // 删除挑战，防止重用
    reply = RedisModule_Call(ctx, "DEL", "s", key_name);
    RedisModule_FreeCallReply(reply);
    RedisModule_FreeString(ctx, key_name);
    
    // 存储令牌 - 修改为直接使用字符串常量
    RedisModuleString *token_key = RedisModule_CreateStringPrintf(ctx, "pow:token:%s", token);
    if (!token_key) {
        return RedisModule_ReplyWithError(ctx, "ERR failed to create token key");
    }
    
    // 修复：使用静态字符串作为值，而不是动态创建
    const char *one_str = "1";
    reply = RedisModule_Call(ctx, "SET", "sc", token_key, one_str);
    
    if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_ERROR) {
        RedisModule_Log(ctx, "warning", "POWVerify: SET failed: %s",
            RedisModule_CallReplyStringPtr(reply, NULL));
        RedisModule_FreeCallReply(reply);
        RedisModule_FreeString(ctx, token_key);
        return RedisModule_ReplyWithError(ctx, "ERR failed to save token");
    }
    RedisModule_FreeCallReply(reply);
    
    // 设置过期时间 - 直接使用正确的格式 "sl"（字符串和长整型）
    reply = RedisModule_Call(ctx, "EXPIRE", "sl", token_key, 3600);
    
    if (RedisModule_CallReplyType(reply) == REDISMODULE_REPLY_ERROR) {
        RedisModule_Log(ctx, "warning", "POWVerify: EXPIRE failed: %s",
            RedisModule_CallReplyStringPtr(reply, NULL));
    }
    RedisModule_FreeCallReply(reply);
    RedisModule_FreeString(ctx, token_key);
    
    // 返回成功和令牌
    RedisModule_ReplyWithArray(ctx, 2);
    RedisModule_ReplyWithSimpleString(ctx, "OK");
    RedisModule_ReplyWithStringBuffer(ctx, token, strlen(token));
    
    RedisModule_Log(ctx, "notice", "POWVerify: Challenge %s verified successfully", challenge_id);
    return REDISMODULE_OK;
}

// POW.TOKEN <token> - 检查令牌是否有效
int POWCheckToken_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 2) {
        return RedisModule_WrongArity(ctx);
    }
    
    size_t token_len;
    const char *token = RedisModule_StringPtrLen(argv[1], &token_len);
    
    // 检查令牌参数是否有效
    if (!token || token_len == 0) {
        return RedisModule_ReplyWithError(ctx, "ERR invalid token");
    }
    
    RedisModule_Log(ctx, "notice", "POWCheckToken: Checking token: %.*s", (int)token_len, token);
    
    // 使用Redis命令API代替直接访问键
    RedisModuleString *token_key = RedisModule_CreateStringPrintf(ctx, "pow:token:%s", token);
    if (!token_key) {
        return RedisModule_ReplyWithError(ctx, "ERR failed to create token key");
    }
    
    RedisModuleCallReply *reply = RedisModule_Call(ctx, "EXISTS", "s", token_key);
    RedisModule_FreeString(ctx, token_key);
    
    if (RedisModule_CallReplyType(reply) != REDISMODULE_REPLY_INTEGER) {
        RedisModule_FreeCallReply(reply);
        return RedisModule_ReplyWithError(ctx, "ERR failed to check token");
    }
    
    long long exists = RedisModule_CallReplyInteger(reply);
    RedisModule_FreeCallReply(reply);
    
    if (exists == 0) {
        RedisModule_ReplyWithSimpleString(ctx, "INVALID");
    } else {
        RedisModule_ReplyWithSimpleString(ctx, "VALID");
    }
    
    return REDISMODULE_OK;
}

// 模块初始化
int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (RedisModule_Init(ctx, "pow", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    
    // 初始化随机数生成器
    srand(time(NULL));
    
    // 注册命令
    if (RedisModule_CreateCommand(ctx, "pow.challenge", 
                                 POWChallenge_RedisCommand,
                                 "write", 1, 1, 1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    
    if (RedisModule_CreateCommand(ctx, "pow.verify", 
                                 POWVerify_RedisCommand,
                                 "write", 1, 1, 1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    
    if (RedisModule_CreateCommand(ctx, "pow.token", 
                                 POWCheckToken_RedisCommand,
                                 "readonly", 1, 1, 1) == REDISMODULE_ERR) {
        return REDISMODULE_ERR;
    }
    
    return REDISMODULE_OK;
}
