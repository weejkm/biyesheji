#include "JSON.h"
#include <string.h>
#include <stdlib.h>

/* 跳过空白字符 */
static const char* json_skip_ws(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
        p++;
    return p;
}

/**
 * 在 JSON 文本中找到指定 key 后面的 value 起始指针
 * 例：{"cmd":123,"msg":"ok"}
 *     JSON_FindKeyValue(json, "cmd") -> 指向 '1'
 *     JSON_FindKeyValue(json, "msg") -> 指向 '"'
 */
static const char* json_find_key_value(const char *json, const char *key)
{
    if (!json || !key) return NULL;

    /* 构造匹配模式："key" */
    char pattern[64];
    size_t key_len = strlen(key);
    if (key_len > 50) return NULL;   // 防御性，避免 pattern 太长

    pattern[0] = '"';
    memcpy(&pattern[1], key, key_len);
    pattern[1 + key_len] = '"';
    pattern[2 + key_len] = '\0';

    /* 在 json 里搜索 "key" */
    const char *p = json;
    while ((p = strstr(p, pattern)) != NULL)
    {
        p += strlen(pattern);  // 跳到 "key" 后面

        p = json_skip_ws(p);
        if (*p != ':')
        {
            // 有可能是别的上下文里的 "key"，继续找
            continue;
        }
        p++;  // 跳过 ':'
        p = json_skip_ws(p);
        return p;  // 这里就是值的起始
    }

    return NULL;
}

bool JSON_HasKey(const char *json, const char *key)
{
    return (json_find_key_value(json, key) != NULL);
}

bool JSON_GetInt(const char *json, const char *key, int *out)
{
    if (!json || !key || !out) return false;

    const char *p = json_find_key_value(json, key);
    if (!p) return false;

    /* 允许前面有 + 或 - 号 */
    char *endptr;
    long v = strtol(p, &endptr, 10);
    if (endptr == p)  // 没有任何数字
        return false;

    *out = (int)v;
    return true;
}

bool JSON_GetFloat(const char *json, const char *key, float *out)
{
    if (!json || !key || !out) return false;

    const char *p = json_find_key_value(json, key);
    if (!p) return false;

    char *endptr;
    float v = (float)strtod(p, &endptr);
    if (endptr == p)
        return false;

    *out = v;
    return true;
}

bool JSON_GetBool(const char *json, const char *key, bool *out)
{
    if (!json || !key || !out) return false;

    const char *p = json_find_key_value(json, key);
    if (!p) return false;

    if (strncmp(p, "true", 4) == 0)
    {
        *out = true;
        return true;
    }
    else if (strncmp(p, "false", 5) == 0)
    {
        *out = false;
        return true;
    }
    return false;
}

bool JSON_GetString(const char *json, const char *key,
                    char *out, uint16_t out_sz)
{
    if (!json || !key || !out || out_sz == 0) return false;

    const char *p = json_find_key_value(json, key);
    if (!p || *p != '"') return false;

    p++;  // 跳过第一个引号

    uint16_t i = 0;
    while (*p && *p != '"' && i < out_sz - 1)
    {
        if (*p == '\\' && p[1] != '\0')  // 简单处理转义：\x
        {
            p++; // 直接跳过反斜杠，只复制后面那个
        }
        out[i++] = *p++;
    }
    out[i] = '\0';

    if (*p != '"')
    {
        // 字符串没正常结束
        return false;
    }

    return true;
}

bool JSON_GetRaw(const char *json, const char *key,
                 char *out, uint16_t out_sz)
{
    if (!json || !key || !out || out_sz == 0) return false;

    const char *p = json_find_key_value(json, key);
    if (!p) return false;

    /* 如果是字符串，就包含整个 "xxx" */
    if (*p == '"')
    {
        const char *start = p;
        p++; // 跳过开头引号
        while (*p)
        {
            if (*p == '\\' && p[1] != '\0')
            {
                p += 2; // 跳过转义
                continue;
            }
            if (*p == '"') // 结束引号
            {
                p++;
                break;
            }
            p++;
        }

        uint16_t len = (uint16_t)(p - start);
        if (len >= out_sz) len = out_sz - 1;
        memcpy(out, start, len);
        out[len] = '\0';
        return true;
    }
    else
    {
        /* 数字 / 布尔 / null 等：读到 , 或 } 为止 */
        const char *start = p;
        while (*p &&
               *p != ',' &&
               *p != '}' &&
               *p != '\r' &&
               *p != '\n')
        {
            p++;
        }

        /* 去掉末尾可能的空白 */
        const char *end = p;
        while (end > start &&
               (end[-1] == ' ' || end[-1] == '\t'))
        {
            end--;
        }

        uint16_t len = (uint16_t)(end - start);
        if (len >= out_sz) len = out_sz - 1;
        memcpy(out, start, len);
        out[len] = '\0';
        return true;
    }
}
