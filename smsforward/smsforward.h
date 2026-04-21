#ifndef SMSFORWARD_H
#define SMSFORWARD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include "json.h"

// 配置结构体 - 优化内存占用
typedef struct {
    char enable[8];
    char push_type[8];
    char pushplus_token[128];      // 256→128
    char wxpusher_token[128];      // 256→128
    char wxpusher_topicids[128];   // 256→128
    char pushdeer_key[128];        // 256→128
    char wxpusher_webhook[128];    // 256→128
    char bark_did[128];            // 256→128
    char dingtalk_token[128];      // 256→128
    char feishu_webhook[128];      // 256→128
    char iyuu_token[128];          // 256→128
    char telegram_bot_token[128];  // Telegram Bot Token
    char telegram_chat_id[128];    // Telegram Chat ID
    char serverchan_sendkey[128];  // ServerChan SendKey
    char sms_title[128];           // 256→128
    char at_port[64];
    char at_mode[32];
} config_t;

// 短信结构体 - 极致优化内存占用
typedef struct {
    int index;
    char sender[32];       // 64→32，一般号码不超过20位
    long timestamp;
    char content[256];     // 512→256，一般短信70字(210字节)
    int reference;         // 长短信标识（同一条短信的各部分有相同的reference）
    int total;             // 长短信总片数
    int part;              // 当前是第几片
} sms_t;

// 短信数据结构体
typedef struct {
    sms_t *messages;
    int count;
} sms_data_t;

// 全局变量
extern config_t g_config;
extern char g_lock_file[];
extern char g_record_file[];
extern char g_summary_file[];

// 函数声明

// 配置相关
int read_config(const char *config_path, config_t *config);
void init_default_config(config_t *config);

// 文件锁相关
int check_lock(const char *lock_file);
void remove_lock(const char *lock_file);

// 短信相关
char* get_modem_device_id(const char *config_file);
int get_sms_storage_count(const char *resolved_port, int *storage_count);
int get_sms_data(sms_data_t *sms_data, const char *resolved_port);
int is_sms_data_valid(const sms_data_t *sms_data, const sms_data_t *last_sms_data);
void format_sms_message(const sms_t *sms, char *buffer, size_t buffer_size);
sms_t* get_latest_sms_list(const sms_data_t *sms_data, int *count, int max_count);
long get_max_timestamp(const sms_data_t *sms_data);

// 文件操作
int load_last_sms_data(const char *filename, sms_data_t *sms_data);
int save_last_sms_data(const char *filename, const sms_data_t *sms_data);
void write_summary_to_file(int count, const char *content);

// HTTP推送相关
int push_pushplus(const char *message, const char *token, const char *title);
int push_wxpusher(const char *message, const char *token, const char *topicids, const char *title);
int push_pushdeer(const char *message, const char *pushdeer_key, const char *title);
int push_wxpusher_webhook(const char *message, const char *wxpusher_webhook, const char *title);
int push_bark(const char *message, const char *bark_did, const char *title);
int push_dingtalk(const char *message, const char *dingtalk_token, const char *title);
int push_feishu(const char *message, const char *feishu_webhook, const char *title);
int push_iyuu(const char *message, const char *iyuu_token, const char *title);
int push_telegram(const char *message, const char *bot_token, const char *chat_id, const char *title);
int push_serverchan(const char *message, const char *sendkey, const char *title);

// 工具函数
char* trim_whitespace(char *str);
int str_contains(const char *haystack, const char *needle);
void free_sms_data(sms_data_t *sms_data);
int compare_sms_timestamp(const void *a, const void *b);
char* merge_multipart_sms(const sms_data_t *sms_data, int reference, char *sender, long *timestamp);
int merge_all_multipart_sms(const sms_data_t *input, sms_data_t *output);

// 备用解析函数
int parse_sms_fallback(const char *buffer, sms_data_t *sms_data);

// 主函数
void forward(void);

#endif // SMSFORWARD_H
