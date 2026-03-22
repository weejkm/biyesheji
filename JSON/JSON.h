#ifndef __JSON_H__
#define __JSON_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  从一段 JSON 文本中读取 int
 * @param  json    JSON 字符串（必须是以 \0 结尾）
 * @param  key     要查找的键名，不带引号，比如 "cmd"
 * @param  out     输出的整数指针
 * @return true 成功找到并解析，false 未找到或格式错误
 */
bool JSON_GetInt(const char *json, const char *key, int *out);

/**
 * @brief  从一段 JSON 文本中读取 float
 * @param  json    JSON 字符串
 * @param  key     键名
 * @param  out     输出的 float 指针
 * @return true 成功，false 失败
 */
bool JSON_GetFloat(const char *json, const char *key, float *out);

/**
 * @brief  从一段 JSON 文本中读取 bool
 * @param  json    JSON 字符串
 * @param  key     键名
 * @param  out     输出的 bool 指针
 * @return true 成功，false 失败
 */
bool JSON_GetBool(const char *json, const char *key, bool *out);

/**
 * @brief  从一段 JSON 文本中读取字符串
 * @param  json    JSON 字符串
 * @param  key     键名
 * @param  out     输出缓冲区
 * @param  out_sz  输出缓冲区大小（字节）
 * @return true 成功，false 失败
 */
bool JSON_GetString(const char *json, const char *key,
                    char *out, uint16_t out_sz);

/**
 * @brief  判断 JSON 中是否存在某个 key（不关心值类型）
 * @param  json    JSON 字符串
 * @param  key     键名
 * @return true 存在，false 不存在
 */
bool JSON_HasKey(const char *json, const char *key);

/**
 * @brief  取得某个 key 对应的“原始值字符串”（不去掉引号）
 *         比如 "cmd":123  -> valueStr="123"
 *              "msg":"hi" -> valueStr="\"hi\""
 * @param  json      JSON 字符串
 * @param  key       键名
 * @param  out       输出缓冲区
 * @param  out_sz    缓冲区大小
 * @return true 成功，false 失败
 */
bool JSON_GetRaw(const char *json, const char *key,
                 char *out, uint16_t out_sz);

#endif /* __JSON_H__ */
