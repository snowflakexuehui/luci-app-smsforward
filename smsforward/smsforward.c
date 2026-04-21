#include "smsforward.h"
#include "serial.h"

#include <glob.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <strings.h>
#include <sys/time.h>
#include <termios.h>
#include <fcntl.h>

// 全局变量定义
config_t g_config;
char g_lock_file[] = "/tmp/smsforward.lock";
char g_record_file[] = "/tmp/last_sms_data.txt";
char g_summary_file[] = "/tmp/smsforwardsum.conf";

#define DEFAULT_AT_BAUDRATE 115200
#define AT_CMD_SHORT_TIMEOUT_MS 2000
#define AT_CMD_LONG_TIMEOUT_MS 8000
#define MAX_AT_RESPONSE_SIZE 65536

typedef struct {
    int use_tom_modem;
    int fd;
    char port[64];
} at_client_t;

// 初始化默认配置
void init_default_config(config_t *config) {
    strcpy(config->enable, "0");
    strcpy(config->push_type, "0");
    strcpy(config->pushplus_token, "");
    strcpy(config->wxpusher_token, "");
    strcpy(config->wxpusher_topicids, "");
    strcpy(config->pushdeer_key, "");
    strcpy(config->wxpusher_webhook, "");
    strcpy(config->bark_did, "");
    strcpy(config->dingtalk_token, "");
    strcpy(config->feishu_webhook, "");
    strcpy(config->iyuu_token, "");
    strcpy(config->telegram_bot_token, "");
    strcpy(config->telegram_chat_id, "");
    strcpy(config->serverchan_sendkey, "");
    strcpy(config->sms_title, "CPE短信转发标题未定义");
    strcpy(config->at_port, "qmodem");
    strcpy(config->at_mode, "tom_modem");
}

// 读取配置文件
int read_config(const char *config_path, config_t *config) {
    FILE *file;
    char line[512];
    char key[64], value[256];
    int found_keys = 0;
    
    init_default_config(config);
    
    file = fopen(config_path, "r");
    if (!file) {
        printf("未找到配置文件: %s，程序已退出！\n", config_path);
        return -1;
    }
    
    printf("正在读取配置文件: %s\n", config_path);
    
    while (fgets(line, sizeof(line), file)) {
        // 跳过注释和空行
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        
        // 去除行尾的换行符
        line[strcspn(line, "\r\n")] = 0;
        
        if (sscanf(line, " option %63s '%255[^']'", key, value) == 2) {
            printf("解析配置项: %s = %s\n", key, value);
            if (strcmp(key, "enable") == 0) {
                strcpy(config->enable, value);
                found_keys++;
            } else if (strcmp(key, "push_type") == 0) {
                strcpy(config->push_type, value);
                found_keys++;
            } else if (strcmp(key, "pushplus_token") == 0) {
                strcpy(config->pushplus_token, value);
                found_keys++;
            } else if (strcmp(key, "wxpusher_token") == 0) {
                strcpy(config->wxpusher_token, value);
                found_keys++;
            } else if (strcmp(key, "wxpusher_topicids") == 0) {
                strcpy(config->wxpusher_topicids, value);
                found_keys++;
            } else if (strcmp(key, "pushdeer_key") == 0) {
                strcpy(config->pushdeer_key, value);
                found_keys++;
            } else if (strcmp(key, "wxpusher_webhook") == 0) {
                strcpy(config->wxpusher_webhook, value);
                found_keys++;
            } else if (strcmp(key, "bark_did") == 0) {
                strcpy(config->bark_did, value);
                found_keys++;
            } else if (strcmp(key, "dingtalk_token") == 0) {
                strcpy(config->dingtalk_token, value);
                found_keys++;
            } else if (strcmp(key, "feishu_webhook") == 0) {
                strcpy(config->feishu_webhook, value);
                found_keys++;
            } else if (strcmp(key, "iyuu_token") == 0) {
                strcpy(config->iyuu_token, value);
                found_keys++;
            } else if (strcmp(key, "telegram_bot_token") == 0) {
                strcpy(config->telegram_bot_token, value);
                found_keys++;
            } else if (strcmp(key, "telegram_chat_id") == 0) {
                strcpy(config->telegram_chat_id, value);
                found_keys++;
            } else if (strcmp(key, "serverchan_sendkey") == 0) {
                strcpy(config->serverchan_sendkey, value);
                found_keys++;
            } else if (strcmp(key, "sms_title") == 0) {
                strcpy(config->sms_title, value);
                found_keys++;
            } else if (strcmp(key, "at_port") == 0) {
                strncpy(config->at_port, value, sizeof(config->at_port) - 1);
                config->at_port[sizeof(config->at_port) - 1] = '\0';
                found_keys++;
            } else if (strcmp(key, "at_mode") == 0 || strcmp(key, "at_modem") == 0) {
                strncpy(config->at_mode, value, sizeof(config->at_mode) - 1);
                config->at_mode[sizeof(config->at_mode) - 1] = '\0';
                found_keys++;
            }
        }
    }
    
    fclose(file);
    return found_keys;
}

// 检查文件锁
int check_lock(const char *lock_file) {
    FILE *file;
    pid_t pid;
    
    if (access(lock_file, F_OK) == 0) {
        printf("脚本已经在运行中。\n");
        return 0;
    }
    
    file = fopen(lock_file, "w");
    if (!file) {
        printf("无法创建锁文件: %s\n", strerror(errno));
        return 0;
    }
    
    fprintf(file, "%d", getpid());
    fclose(file);
    return 1;
}

// 移除文件锁
void remove_lock(const char *lock_file) {
    if (unlink(lock_file) != 0) {
        printf("无法删除锁文件: %s\n", strerror(errno));
    }
}

// 获取modem设备ID
char* get_modem_device_id(const char *config_file) {
    static char device_id[64] = {0};
    FILE *file;
    char line[256];
    
    file = fopen(config_file, "r");
    if (!file) {
        printf("读取配置文件异常: %s\n", strerror(errno));
        return NULL;
    }
    
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "config modem-device '%63[^']'", device_id) == 1) {
            fclose(file);
            return device_id;
        }
    }
    
    fclose(file);
    return NULL;
}

// 获取modem设备at端口（优先使用sms_at_port，如果没有则使用at_port）
char* get_modem_device_at_port(const char *config_file) {
    static char at_port[64] = {0};
    static char sms_at_port[64] = {0};
    FILE *file;
    char line[512];
    int in_modem_device = 0;
    int found_sms_at_port = 0;
    int found_at_port = 0;
    
    file = fopen(config_file, "r");
    if (!file) {
        printf("读取配置文件异常: %s\n", strerror(errno));
        return NULL;
    }
    
    // 扫描配置文件，查找sms_at_port和at_port
    while (fgets(line, sizeof(line), file)) {
        // 去掉换行符
        line[strcspn(line, "\n\r")] = 0;
        
        // 检查是否进入modem-device配置段
        if (strstr(line, "config modem-device")) {
            in_modem_device = 1;
            continue;
        }
        
        // 如果遇到新的config段，退出当前段
        if (in_modem_device && strstr(line, "config ") && !strstr(line, "config modem-device")) {
            in_modem_device = 0;
        }
        
        // 在modem-device段中查找sms_at_port（优先）
        if (in_modem_device && strstr(line, "option sms_at_port")) {
            // 提取引号中的串口路径
            char *start = strchr(line, '\'');
            if (!start) start = strchr(line, '"');
            if (start) {
                start++; // 跳过引号
                char *end = strchr(start, '\'');
                if (!end) end = strchr(start, '"');
                if (end) {
                    int len = end - start;
                    if (len > 0 && len < (int)sizeof(sms_at_port)) {
                        strncpy(sms_at_port, start, len);
                        sms_at_port[len] = '\0';
                        found_sms_at_port = 1;
                    }
                }
            }
        }
        
        // 在modem-device段中查找at_port（备用，但排除sms_at_port）
        if (in_modem_device && strstr(line, "option at_port") && !strstr(line, "sms_at_port")) {
            // 提取引号中的串口路径
            char *start = strchr(line, '\'');
            if (!start) start = strchr(line, '"');
            if (start) {
                start++; // 跳过引号
                char *end = strchr(start, '\'');
                if (!end) end = strchr(start, '"');
                if (end) {
                    int len = end - start;
                    if (len > 0 && len < (int)sizeof(at_port)) {
                        strncpy(at_port, start, len);
                        at_port[len] = '\0';
                        found_at_port = 1;
                    }
                }
            }
        }
    }
    
    fclose(file);
    
    // 优先返回sms_at_port，如果没有则返回at_port
    if (found_sms_at_port) {
        printf("从qmodem配置读取sms_at_port: %s\n", sms_at_port);
        return sms_at_port;
    } else if (found_at_port) {
        printf("从qmodem配置读取at_port: %s\n", at_port);
        return at_port;
    }
    
    return NULL;
}

static long long now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}


static int hexchar_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static int hexpair_to_int(const char *p) {
    int hi = hexchar_to_int(p[0]);
    int lo = hexchar_to_int(p[1]);
    if (hi < 0 || lo < 0) return -1;
    return (hi << 4) | lo;
}

static int hex_to_bytes(const char *hex, uint8_t **out, size_t *out_len) {
    size_t hex_len = strlen(hex);
    if (hex_len == 0 || (hex_len % 2) != 0) {
        return -1;
    }
    size_t byte_len = hex_len / 2;
    uint8_t *buf = malloc(byte_len);
    if (!buf) {
        return -1;
    }
    for (size_t i = 0; i < byte_len; i++) {
        int val = hexpair_to_int(&hex[i * 2]);
        if (val < 0) {
            free(buf);
            return -1;
        }
        buf[i] = (uint8_t)val;
    }
    *out = buf;
    if (out_len) *out_len = byte_len;
    return 0;
}

static void normalize_phone_number(char *number) {
    if (!number) return;
    if (strncmp(number, "86", 2) == 0) {
        memmove(number, number + 2, strlen(number) - 1);
    }
    while (number[0] == '0' && number[1] != '\0') {
        memmove(number, number + 1, strlen(number));
    }
}

static int test_at_device(const char *device) {
    if (!device) return 0;
    
    // 跳过控制台设备，这些通常不是AT设备且可能导致阻塞
    if (strcmp(device, "/dev/ttyS0") == 0 || strcmp(device, "/dev/console") == 0) {
        return 0;
    }
    
    int fd = serialOpen(device, DEFAULT_AT_BAUDRATE);
    if (fd < 0) {
        return 0;
    }
    
    // 清空缓冲区
    tcflush(fd, TCIOFLUSH);
    
    // 发送AT命令（使用 serialWriteString，它会自动添加 \r）
    serialWriteString(fd, "AT\r");
    
    char buffer[256];
    int total_len = 0;
    buffer[0] = '\0';
    
    // 循环接收，最多等待1秒（与 webuiserver 保持一致）
    const int max_wait_time = 1; // 最多等待1秒
    const int check_interval = 100000; // 每100ms检查一次
    int waited_time = 0;
    
    while (waited_time < max_wait_time * 1000000) { // 转换为微秒
        char chunk[64];
        // 使用 serialReadRaw，它有内置的超时保护（TIMEOUT=2秒，但每次调用最多等待2秒）
        int len = serialReadRaw(fd, chunk, sizeof(chunk) - 1);
        
        if (len > 0) {
            chunk[len] = '\0';
            // 将新数据追加到缓冲区
            if (total_len + len < sizeof(buffer) - 1) {
                strcat(buffer + total_len, chunk);
                total_len += len;
                buffer[total_len] = '\0';
            }
            
            // 检查是否收到AT响应
            if (strstr(buffer, "OK") != NULL) {
                serialClose(fd);
                return 1;
            }
        }
        
        usleep(check_interval);
        waited_time += check_interval;
    }
    
    serialClose(fd);
    return 0;
}

static int detect_serial_device(char *out, size_t len) {
    // 优先测试USB设备，这些更可能是AT设备
    const char *patterns[] = {
        "/dev/ttyUSB*",
        "/dev/ttyACM*",
        "/dev/ttyAMA*",
        "/dev/ttyS*"
    };
    int pattern_count = sizeof(patterns) / sizeof(patterns[0]);
    int max_devices_per_pattern = 10; // 限制每个模式最多测试10个设备
    
    for (int i = 0; i < pattern_count; i++) {
        glob_t result;
        if (glob(patterns[i], GLOB_NOSORT, NULL, &result) != 0) {
            continue;
        }
        
        // 限制测试的设备数量，避免测试太多设备导致卡住
        size_t test_count = (result.gl_pathc > max_devices_per_pattern) ? 
                           max_devices_per_pattern : result.gl_pathc;
        
        for (size_t j = 0; j < test_count; j++) {
            printf("测试设备: %s\n", result.gl_pathv[j]);
            if (test_at_device(result.gl_pathv[j])) {
                printf("找到AT设备: %s\n", result.gl_pathv[j]);
                strncpy(out, result.gl_pathv[j], len - 1);
                out[len - 1] = '\0';
                globfree(&result);
                return 0;
            }
        }
        globfree(&result);
    }
    return -1;
}

static int read_qmodem_port(char *out, size_t len) {
    char *cfg_port = get_modem_device_at_port("/etc/config/qmodem");
    if (!cfg_port) {
        return -1;
    }
    strncpy(out, cfg_port, len - 1);
    out[len - 1] = '\0';
    return 0;
}

static int resolve_at_port(char *out, size_t len) {
    if (!out || len == 0) {
        return -1;
    }
    out[0] = '\0';
    
    if (strlen(g_config.at_port) == 0 || strcmp(g_config.at_port, "auto") == 0) {
        if (detect_serial_device(out, len) == 0) {
            printf("自动检测到串口: %s\n", out);
            return 0;
        }
        printf("自动检测串口失败，尝试使用默认串口。\n");
    } else if (strcmp(g_config.at_port, "qmodem") == 0) {
        if (read_qmodem_port(out, len) == 0) {
            printf("从qmodem配置读取串口: %s\n", out);
            return 0;
        }
        printf("读取qmodem串口失败。\n");
    } else {
        strncpy(out, g_config.at_port, len - 1);
        out[len - 1] = '\0';
        return 0;
    }
    
    strncpy(out, "/dev/ttyUSB0", len - 1);
    out[len - 1] = '\0';
    return 0;
}

static int at_client_open(at_client_t *client, const char *mode, const char *port) {
    if (!client || !port) return -1;
    memset(client, 0, sizeof(*client));
    strncpy(client->port, port, sizeof(client->port) - 1);
    client->use_tom_modem = 1;
    client->fd = -1;
    
    if (mode && strcasecmp(mode, "direct") == 0) {
        client->use_tom_modem = 0;
        client->fd = serialOpen(port, DEFAULT_AT_BAUDRATE);
        if (client->fd < 0) {
            printf("打开串口失败: %s\n", port);
            return -1;
        }
        tcflush(client->fd, TCIOFLUSH);
    }
    return 0;
}

static void at_client_close(at_client_t *client) {
    if (!client) return;
    if (!client->use_tom_modem && client->fd >= 0) {
        serialClose(client->fd);
        client->fd = -1;
    }
}

static int at_client_command(at_client_t *client, const char *cmd, char **response, int long_timeout) {
    if (!client || !cmd || !response) {
        return -1;
    }
    *response = NULL;
    size_t capacity = 0;
    size_t size = 0;
    int timeout_ms = long_timeout ? AT_CMD_LONG_TIMEOUT_MS : AT_CMD_SHORT_TIMEOUT_MS;
    
    if (client->use_tom_modem) {
        char shell_cmd[512];
        snprintf(shell_cmd, sizeof(shell_cmd), "tom_modem -c '%s' -d '%s' -b %d 2>/dev/null",
                 cmd, client->port, DEFAULT_AT_BAUDRATE);
        FILE *fp = popen(shell_cmd, "r");
        if (!fp) {
            return -1;
        }
        char buf[512];
        while (!feof(fp)) {
            size_t n = fread(buf, 1, sizeof(buf), fp);
            if (n > 0) {
                if (size + n + 1 > capacity) {
                    size_t new_cap = (capacity == 0) ? (n + 1024) : capacity * 2;
                    while (new_cap < size + n + 1) new_cap *= 2;
                    char *tmp = realloc(*response, new_cap);
                    if (!tmp) {
                        pclose(fp);
                        free(*response);
                        *response = NULL;
                        return -1;
                    }
                    *response = tmp;
                    capacity = new_cap;
                }
                memcpy(*response + size, buf, n);
                size += n;
            }
        }
        pclose(fp);
        if (!*response) {
            *response = calloc(1, 1);
            capacity = 1;
        }
        (*response)[size] = '\0';
        return 0;
    }
    
    if (client->fd < 0) {
        return -1;
    }
    
    char command_buf[256];
    snprintf(command_buf, sizeof(command_buf), "%s\r", cmd);
    serialWriteRaw(client->fd, command_buf, strlen(command_buf));
    serialWaitUntilSent(client->fd);
    
    *response = malloc(2048);
    if (!*response) {
        return -1;
    }
    capacity = 2048;
    size = 0;
    (*response)[0] = '\0';
    
    long long start = now_ms();
    while (now_ms() - start < timeout_ms) {
        char chunk[512];
        int len = serialReadRaw(client->fd, chunk, sizeof(chunk) - 1);
        if (len > 0) {
            if (size + len + 1 > capacity) {
                size_t new_cap = capacity * 2;
                while (new_cap < size + len + 1) new_cap *= 2;
                char *tmp = realloc(*response, new_cap);
                if (!tmp) {
                    free(*response);
                    *response = NULL;
                    return -1;
                }
                *response = tmp;
                capacity = new_cap;
            }
            memcpy(*response + size, chunk, len);
            size += len;
            (*response)[size] = '\0';
            if (strstr(*response, "OK") || strstr(*response, "ERROR")) {
                break;
            }
        } else {
            usleep(100000);
        }
    }
    return 0;
}

static int parse_cpms_count(const char *resp, int *count) {
    if (!resp || !count) return -1;
    const char *ptr = strstr(resp, "+CPMS:");
    if (!ptr) return -1;
    ptr = strchr(ptr, ',');
    if (!ptr) return -1;
    ptr++; // move to number
    while (*ptr == ' ' || *ptr == '"') ptr++;
    *count = atoi(ptr);
    return 0;
}

static void append_utf8_char(char *out, size_t *len, size_t max_len, uint16_t codepoint) {
    if (!out || !len || *len >= max_len - 1) return;
    if (codepoint < 0x80) {
        out[(*len)++] = (char)codepoint;
    } else if (codepoint < 0x800) {
        if (*len + 2 >= max_len) return;
        out[(*len)++] = 0xC0 | (codepoint >> 6);
        out[(*len)++] = 0x80 | (codepoint & 0x3F);
    } else {
        if (*len + 3 >= max_len) return;
        out[(*len)++] = 0xE0 | (codepoint >> 12);
        out[(*len)++] = 0x80 | ((codepoint >> 6) & 0x3F);
        out[(*len)++] = 0x80 | (codepoint & 0x3F);
    }
    out[*len] = '\0';
}

static int decode_ucs2(const uint8_t *data, int byte_len, char *out, size_t out_size) {
    if (!data || !out || out_size == 0) return -1;
    size_t pos = 0;
    for (int i = 0; i + 1 < byte_len; i += 2) {
        uint16_t code = ((uint16_t)data[i] << 8) | data[i + 1];
        append_utf8_char(out, &pos, out_size, code);
    }
    return 0;
}

static int decode_gsm7(const uint8_t *data, int septet_len, char *out, size_t out_size) {
    if (!data || !out || out_size == 0) return -1;
    size_t pos = 0;
    int byte_len = (septet_len * 7 + 7) / 8;
    for (int i = 0; i < septet_len && pos < out_size - 1; i++) {
        int bit_offset = (i * 7) % 8;
        int byte_offset = (i * 7) / 8;
        uint16_t val = (data[byte_offset] >> bit_offset) & 0x7F;
        if (bit_offset > 1 && byte_offset + 1 < byte_len) {
            val |= (data[byte_offset + 1] << (8 - bit_offset)) & 0x7F;
        }
        if (val == 0x00) {
            out[pos++] = '@';
        } else if (val == 0x0D) {
            out[pos++] = '\r';
        } else if (val == 0x0A) {
            out[pos++] = '\n';
        } else {
            out[pos++] = (val >= 32 && val <= 126) ? (char)val : ' ';
        }
    }
    out[pos] = '\0';
    return 0;
}

static int parse_concat_info(const uint8_t *udh, int udh_len, int *ref, int *total, int *seq) {
    if (!udh || udh_len <= 0) return 0;
    int idx = 0;
    while (idx + 2 <= udh_len) {
        uint8_t iei = udh[idx++];
        uint8_t ie_len = udh[idx++];
        if (idx + ie_len > udh_len) break;
        if (iei == 0x00 && ie_len == 3) {
            if (ref) *ref = udh[idx];
            if (total) *total = udh[idx + 1];
            if (seq) *seq = udh[idx + 2];
            return 1;
        } else if (iei == 0x08 && ie_len == 4) {
            if (ref) *ref = (udh[idx] << 8) | udh[idx + 1];
            if (total) *total = udh[idx + 2];
            if (seq) *seq = udh[idx + 3];
            return 1;
        }
        idx += ie_len;
    }
    return 0;
}

// 完全按照JavaScript的swapSemiOctet逻辑
// swapSemiOctet交换每两个字符：res += (str[i + 1] || '') + str[i];
// 例如："08" → "80", "52" → "25"
// 输入：两个字节的十六进制字符串（如"08"）
// 输出：交换后的字符串解析为整数（如"80"解析为80）
static int swap_semi_octet_parse_byte(uint8_t byte) {
    char hex_str[3];
    snprintf(hex_str, sizeof(hex_str), "%02X", byte);
    // swapSemiOctet逻辑：交换每两个字符
    char tmp = hex_str[0];
    hex_str[0] = hex_str[1];
    hex_str[1] = tmp;
    // 解析为十进制整数（不是十六进制）
    return (int)strtol(hex_str, NULL, 10);
}

static time_t timegm_compat(struct tm *tm_info) {
#if defined(_GNU_SOURCE) || defined(__USE_BSD) || defined(__APPLE__)
    return timegm(tm_info);
#else
    char *tz = getenv("TZ");
    char *tz_backup = tz ? strdup(tz) : NULL;
    setenv("TZ", "UTC", 1);
    tzset();
    time_t result = mktime(tm_info);
    if (tz_backup) {
        setenv("TZ", tz_backup, 1);
        free(tz_backup);
    } else {
        unsetenv("TZ");
    }
    tzset();
    return result;
#endif
}

// 完全按照JavaScript的decodeTimestamp逻辑
// decodeTimestamp(ts: string) {
//     const year = '20' + swapSemiOctet(ts.slice(0, 2));
//     const month = swapSemiOctet(ts.slice(2, 4));
//     const day = swapSemiOctet(ts.slice(4, 6));
//     const hour = swapSemiOctet(ts.slice(6, 8));
//     const min = swapSemiOctet(ts.slice(8, 10));
//     const sec = swapSemiOctet(ts.slice(10, 12));
//     return `${year}-${month}-${day} ${hour}:${min}:${sec}`;
// }
static int decode_timestamp_to_epoch(const uint8_t *scts, long *timestamp_out) {
    if (!scts || !timestamp_out) return -1;
    struct tm tm_info;
    memset(&tm_info, 0, sizeof(tm_info));
    
    // 完全按照JavaScript代码的逻辑
    // scts是7个字节，对应14个十六进制字符
    // ts.slice(0, 2) 对应 scts[0]
    // ts.slice(2, 4) 对应 scts[1]
    // 等等
    int year = 2000 + swap_semi_octet_parse_byte(scts[0]); // '20' + swapSemiOctet(ts.slice(0, 2))
    int month = swap_semi_octet_parse_byte(scts[1]);      // swapSemiOctet(ts.slice(2, 4))
    int day = swap_semi_octet_parse_byte(scts[2]);         // swapSemiOctet(ts.slice(4, 6))
    int hour = swap_semi_octet_parse_byte(scts[3]);        // swapSemiOctet(ts.slice(6, 8))
    int minute = swap_semi_octet_parse_byte(scts[4]);      // swapSemiOctet(ts.slice(8, 10))
    int second = swap_semi_octet_parse_byte(scts[5]);      // swapSemiOctet(ts.slice(10, 12))
    
    // 时区：ts.slice(12, 14) 对应 scts[6]
    uint8_t tz_swapped = ((scts[6] & 0x0F) << 4) | ((scts[6] >> 4) & 0x0F);
    int tz_units = tz_swapped & 0x0F;
    int tz_tens = (tz_swapped >> 4) & 0x07;
    int tz_sign = (tz_swapped & 0x80) ? -1 : 1;
    int tz_quarters = tz_units + tz_tens * 10;
    int tz_offset_seconds = tz_quarters * 15 * 60 * tz_sign;
    
    tm_info.tm_year = year - 1900; // struct tm使用1900作为基准年
    tm_info.tm_mon = (month > 0 ? month - 1 : 0);
    tm_info.tm_mday = (day > 0 ? day : 1);
    tm_info.tm_hour = hour;
    tm_info.tm_min = minute;
    tm_info.tm_sec = second;
    tm_info.tm_isdst = 0;
    
    time_t utc_time = timegm_compat(&tm_info);
    *timestamp_out = (long)utc_time;
    
    // 调试：打印解析结果
    printf("[DEBUG] 时间戳解析: %04d-%02d-%02d %02d:%02d:%02d -> %ld\n",
           year, month, day, hour, minute, second, *timestamp_out);
    
    return 0;
}

static int decode_sms_pdu(const char *pdu_hex, sms_t *sms) {
    if (!pdu_hex || !sms) return -1;
    uint8_t *bytes = NULL;
    size_t len = 0;
    if (hex_to_bytes(pdu_hex, &bytes, &len) != 0) {
        return -1;
    }
    size_t idx = 0;
    if (len < 2) {
        free(bytes);
        return -1;
    }
    int smsc_len = bytes[idx++];
    if (idx + smsc_len > len) {
        free(bytes);
        return -1;
    }
    idx += smsc_len;
    if (idx >= len) {
        free(bytes);
        return -1;
    }
    uint8_t pdu_type = bytes[idx++];
    int has_udh = (pdu_type & 0x40) != 0;
    if (idx >= len) {
        free(bytes);
        return -1;
    }
    int oa_len_digits = bytes[idx++];
    if (idx >= len) {
        free(bytes);
        return -1;
    }
    idx++; // skip oa type
    int oa_len_bytes = (oa_len_digits + 1) / 2;
    if (idx + oa_len_bytes > len) {
        free(bytes);
        return -1;
    }
    char sender[32] = {0};
    int pos = 0;
    for (int i = 0; i < oa_len_bytes && pos < (int)sizeof(sender) - 1; i++) {
        uint8_t b = bytes[idx + i];
        int d1 = b & 0x0F;
        int d2 = (b >> 4) & 0x0F;
        if (d1 <= 9) sender[pos++] = '0' + d1;
        if (d2 <= 9 && pos < (int)sizeof(sender) - 1) sender[pos++] = '0' + d2;
    }
    sender[pos] = '\0';
    normalize_phone_number(sender);
    strncpy(sms->sender, sender, sizeof(sms->sender) - 1);
    sms->sender[sizeof(sms->sender) - 1] = '\0';
    idx += oa_len_bytes;
    
    if (idx + 2 > len) {
        free(bytes);
        return -1;
    }
    idx++; // PID
    uint8_t dcs = bytes[idx++];
    if (idx + 7 > len) {
        free(bytes);
        return -1;
    }
    long timestamp = 0;
    if (decode_timestamp_to_epoch(&bytes[idx], &timestamp) != 0) {
        timestamp = time(NULL);
    }
    sms->timestamp = timestamp;
    idx += 7;
    
    if (idx >= len) {
        free(bytes);
        return -1;
    }
    int udl = bytes[idx++];
    int remaining = len - idx;
    const uint8_t *ud = &bytes[idx];
    int ref = 0, total = 1, part = 1;
    int header_bytes = 0;
    if (has_udh && remaining > 0) {
        int udhl = ud[0];
        if (udhl + 1 <= remaining) {
            parse_concat_info(ud + 1, udhl, &ref, &total, &part);
            header_bytes = udhl + 1;
            ud += header_bytes;
            remaining -= header_bytes;
        }
    }
    sms->reference = ref;
    sms->total = total;
    sms->part = part;
    
    char content[512];
    content[0] = '\0';
    if ((dcs & 0x0C) == 0x08) {
        decode_ucs2(ud, remaining, content, sizeof(content));
    } else if ((dcs & 0x0C) == 0x00) {
        int septet_len = udl;
        if (has_udh && header_bytes < udl) {
            septet_len -= header_bytes;
        }
        if (septet_len < 0) septet_len = 0;
        decode_gsm7(ud, septet_len, content, sizeof(content));
    } else {
        int copy_len = remaining < (int)sizeof(content) - 1 ? remaining : (int)sizeof(content) - 1;
        for (int i = 0; i < copy_len; i++) {
            content[i] = (ud[i] >= 32 && ud[i] <= 126) ? ud[i] : '.';
        }
        content[copy_len] = '\0';
    }
    strncpy(sms->content, content, sizeof(sms->content) - 1);
    sms->content[sizeof(sms->content) - 1] = '\0';
    
    free(bytes);
    return 0;
}

static int parse_cmgl_response(char *resp, sms_data_t *sms_data) {
    if (!resp || !sms_data) return -1;
    char *cursor = resp;
    int estimated = 0;
    while ((cursor = strstr(cursor, "+CMGL:")) != NULL) {
        estimated++;
        cursor += 6;
    }
    if (estimated == 0) {
        sms_data->messages = NULL;
        sms_data->count = 0;
        return 0;
    }
    sms_data->messages = calloc(estimated, sizeof(sms_t));
    if (!sms_data->messages) {
        sms_data->count = 0;
        return -1;
    }
    sms_data->count = 0;
    
    char *line = resp;
    int current_idx = -1;
    while (line && *line) {
        char *next = strpbrk(line, "\r\n");
        if (next) {
            *next = '\0';
            next++;
            while (*next == '\r' || *next == '\n') next++;
        }
        if (strncmp(line, "+CMGL:", 6) == 0) {
            int index = 0;
            sscanf(line, "+CMGL: %d", &index);
            current_idx = sms_data->count;
            sms_data->messages[current_idx].index = index;
            sms_data->messages[current_idx].reference = 0;
            sms_data->messages[current_idx].total = 1;
            sms_data->messages[current_idx].part = 1;
        } else if (line[0] != '\0' && current_idx >= 0) {
            if (decode_sms_pdu(line, &sms_data->messages[current_idx]) == 0) {
                sms_data->messages[current_idx].index = sms_data->messages[current_idx].index == 0
                    ? sms_data->count : sms_data->messages[current_idx].index;
                sms_data->count++;
            }
            current_idx = -1;
        }
        line = next;
    }
    return 0;
}

// 发送HTTP POST请求的通用函数
// HTTP POST通用函数 - 使用wget替代libcurl（节省2-3MB内存）
int send_http_post(const char *url, const char *json_data, char *response_buffer) {
    FILE *fp;
    char *cmd;
    int result;
    size_t cmd_size;
    
    // 动态分配命令缓冲区
    cmd_size = strlen(url) + strlen(json_data) + 256;
    cmd = malloc(cmd_size);
    if (!cmd) {
        return -1;
    }
    
    // 构建wget命令
    snprintf(cmd, cmd_size,
        "wget --post-data='%s' "
        "--header='Content-Type: application/json' "
        "--timeout=10 --tries=1 -4 -q -O- '%s' 2>/dev/null",
        json_data, url);
    
    // 执行命令
    fp = popen(cmd, "r");
    free(cmd);
    
    if (!fp) {
        return -1;
    }
    
    // 读取响应
    if (response_buffer) {
        size_t n = fread(response_buffer, 1, 2047, fp);
        response_buffer[n] = '\0';
    }
    
    result = pclose(fp);
    return (result == 0) ? 0 : -1;
}

// 转义JSON字符串中的特殊字符
void escape_json_string(const char *input, char *output, size_t output_size) {
    size_t j = 0;
    for (size_t i = 0; input[i] && j < output_size - 2; i++) {
        switch (input[i]) {
            case '\n':
                if (j < output_size - 3) {
                    output[j++] = '\\';
                    output[j++] = 'n';
                }
                break;
            case '\r':
                if (j < output_size - 3) {
                    output[j++] = '\\';
                    output[j++] = 'r';
                }
                break;
            case '\t':
                if (j < output_size - 3) {
                    output[j++] = '\\';
                    output[j++] = 't';
                }
                break;
            case '"':
                if (j < output_size - 3) {
                    output[j++] = '\\';
                    output[j++] = '"';
                }
                break;
            case '\\':
                if (j < output_size - 3) {
                    output[j++] = '\\';
                    output[j++] = '\\';
                }
                break;
            default:
                output[j++] = input[i];
                break;
        }
    }
    output[j] = '\0';
}

// PushPlus推送
int push_pushplus(const char *message, const char *token, const char *title) {
    char url[] = "http://www.pushplus.plus/send";
    char *json_data = malloc(8192);  // 增大到8KB
    char *escaped_msg = malloc(4096);  // 用于存储转义后的消息
    char response[4096] = {0};
    int result;
    
    if (!json_data || !escaped_msg) {
        if (json_data) free(json_data);
        if (escaped_msg) free(escaped_msg);
        return -1;
    }
    
    // 转义消息中的特殊字符
    escape_json_string(message, escaped_msg, 4096);
    
    snprintf(json_data, 8192,
        "{\"token\":\"%s\",\"title\":\"%s\",\"content\":\"%s\"}",
        token, title, escaped_msg);
    
    result = send_http_post(url, json_data, response);
    if (result == 0) {
        printf("PushPlus Response:\n%s\n", response);
    } else {
        printf("PushPlus请求失败\n");
    }
    
    free(json_data);
    free(escaped_msg);
    return result;
}

// WxPusher推送
int push_wxpusher(const char *message, const char *token, const char *topicids, const char *title) {
    char url[] = "https://wxpusher.zjiecode.com/api/send/message";
    char *json_data = malloc(8192);
    char *escaped_msg = malloc(4096);
    char response[4096] = {0};
    int result;
    
    if (!json_data || !escaped_msg) {
        if (json_data) free(json_data);
        if (escaped_msg) free(escaped_msg);
        return -1;
    }
    
    escape_json_string(message, escaped_msg, 4096);
    
    snprintf(json_data, 8192,
        "{\"appToken\":\"%s\",\"summary\":\"%s\",\"content\":\"%s\",\"contentType\":1,\"topicIds\":[\"%s\"]}",
        token, title, escaped_msg, topicids);
    
    result = send_http_post(url, json_data, response);
    if (result == 0) {
        printf("WxPusher Response:\n%s\n", response);
    } else {
        printf("WxPusher请求失败\n");
    }
    
    free(json_data);
    free(escaped_msg);
    return result;
}

// PushDeer推送
int push_pushdeer(const char *message, const char *pushdeer_key, const char *title) {
    char url[] = "https://api2.pushdeer.com/message/push";
    char *json_data = malloc(8192);
    char *escaped_msg = malloc(4096);
    char response[4096] = {0};
    int result;
    
    if (!json_data || !escaped_msg) {
        if (json_data) free(json_data);
        if (escaped_msg) free(escaped_msg);
        return -1;
    }
    
    escape_json_string(message, escaped_msg, 4096);
    
    snprintf(json_data, 8192,
        "{\"pushkey\":\"%s\",\"text\":\"%s\",\"desp\":\"%s\",\"type\":\"text\"}",
        pushdeer_key, title, escaped_msg);
    
    result = send_http_post(url, json_data, response);
    if (result == 0) {
        printf("PushDeer Response:\n%s\n", response);
    } else {
        printf("PushDeer请求失败\n");
    }
    
    free(json_data);
    free(escaped_msg);
    return result;
}

// WxPusher Webhook推送
int push_wxpusher_webhook(const char *message, const char *wxpusher_webhook, const char *title) {
    char url[512];
    char *json_data = malloc(8192);
    char *escaped_msg = malloc(4096);
    char response[4096] = {0};
    int result;
    
    if (!json_data || !escaped_msg) {
        if (json_data) free(json_data);
        if (escaped_msg) free(escaped_msg);
        return -1;
    }
    
    escape_json_string(message, escaped_msg, 4096);
    
    snprintf(url, sizeof(url), "https://qyapi.weixin.qq.com/cgi-bin/webhook/send?key=%s", wxpusher_webhook);
    snprintf(json_data, 8192,
        "{\"msgtype\":\"text\",\"text\":{\"content\":\"%s\"}}",
        escaped_msg);
    
    result = send_http_post(url, json_data, response);
    if (result == 0) {
        printf("WxPusher Webhook Response:\n%s\n", response);
    } else {
        printf("WxPusher Webhook请求失败\n");
    }
    
    free(json_data);
    free(escaped_msg);
    return result;
}

// Bark推送
int push_bark(const char *message, const char *bark_did, const char *title) {
    char url[512];
    char *json_data = malloc(8192);
    char *escaped_msg = malloc(4096);
    char response[4096] = {0};
    int result;
    
    if (!json_data || !escaped_msg) {
        if (json_data) free(json_data);
        if (escaped_msg) free(escaped_msg);
        return -1;
    }
    
    escape_json_string(message, escaped_msg, 4096);
    
    snprintf(url, sizeof(url), "https://api.day.app/%s", bark_did);
    snprintf(json_data, 8192,
        "{\"title\":\"%s\",\"body\":\"%s\",\"icon\":\"https://youke1.picui.cn/s1/2025/10/10/68e8a0d83c264.png\"}",
        title, escaped_msg);
    
    result = send_http_post(url, json_data, response);
    if (result == 0) {
        printf("Bark Response:\n%s\n", response);
    } else {
        printf("Bark请求失败\n");
    }
    
    free(json_data);
    free(escaped_msg);
    return result;
}

// DingTalk推送
int push_dingtalk(const char *message, const char *dingtalk_token, const char *title) {
    char url[512];
    char *json_data = malloc(8192);
    char *escaped_msg = malloc(4096);
    char response[4096] = {0};
    int result;
    
    if (!json_data || !escaped_msg) {
        if (json_data) free(json_data);
        if (escaped_msg) free(escaped_msg);
        return -1;
    }
    
    escape_json_string(message, escaped_msg, 4096);
    
    snprintf(url, sizeof(url), "https://oapi.dingtalk.com/robot/send?access_token=%s", dingtalk_token);
    snprintf(json_data, 8192,
        "{\"msgtype\":\"markdown\",\"markdown\":{\"title\":\"%s\",\"text\":\"%s\"}}",
        title, escaped_msg);
    
    result = send_http_post(url, json_data, response);
    if (result == 0) {
        printf("DingTalk Response:\n%s\n", response);
    } else {
        printf("DingTalk请求失败\n");
    }
    
    free(json_data);
    free(escaped_msg);
    return result;
}

// Feishu推送
int push_feishu(const char *message, const char *feishu_webhook, const char *title) {
    char url[512];
    char *json_data = malloc(8192);
    char *escaped_msg = malloc(4096);
    char response[4096] = {0};
    int result;
    
    if (!json_data || !escaped_msg) {
        if (json_data) free(json_data);
        if (escaped_msg) free(escaped_msg);
        return -1;
    }
    
    escape_json_string(message, escaped_msg, 4096);
    
    snprintf(url, sizeof(url), "https://open.feishu.cn/open-apis/bot/v2/hook/%s", feishu_webhook);
    snprintf(json_data, 8192,
        "{\"msg_type\":\"post\",\"content\":{\"post\":{\"zh_cn\":{\"title\":\"%s\",\"content\":[[{\"tag\":\"text\",\"text\":\"%s\"}]]}}}}",
        title, escaped_msg);
    
    result = send_http_post(url, json_data, response);
    if (result == 0) {
        printf("Feishu Response:\n%s\n", response);
    } else {
        printf("Feishu请求失败\n");
    }
    
    free(json_data);
    free(escaped_msg);
    return result;
}

// iyuu推送
int push_iyuu(const char *message, const char *iyuu_token, const char *title) {
    char url[512];
    char *json_data = malloc(8192);
    char *escaped_msg = malloc(4096);
    char response[4096] = {0};
    int result;
    
    if (!json_data || !escaped_msg) {
        if (json_data) free(json_data);
        if (escaped_msg) free(escaped_msg);
        return -1;
    }
    
    escape_json_string(message, escaped_msg, 4096);
    
    snprintf(url, sizeof(url), "https://iyuu.cn/%s.send", iyuu_token);
    snprintf(json_data, 8192,
        "{\"text\":\"%s\",\"desp\":\"%s\"}",
        title, escaped_msg);
    
    result = send_http_post(url, json_data, response);
    if (result == 0) {
        printf("iyuu Response:\n%s\n", response);
    } else {
        printf("iyuu请求失败\n");
    }
    
    free(json_data);
    free(escaped_msg);
    return result;
}

// Telegram推送
int push_telegram(const char *message, const char *bot_token, const char *chat_id, const char *title) {
    char url[512];
    char *json_data = malloc(8192);
    char *escaped_text = malloc(4096);
    char *full_text = malloc(4096);
    char response[4096] = {0};
    int result;
    
    if (!json_data || !escaped_text || !full_text) {
        if (json_data) free(json_data);
        if (escaped_text) free(escaped_text);
        if (full_text) free(full_text);
        return -1;
    }
    
    // 将title和message合并到text中
    snprintf(full_text, 4096, "%s\n\n%s", title, message);
    
    // 转义JSON字符串
    escape_json_string(full_text, escaped_text, 4096);
    
    // 构建URL
    snprintf(url, sizeof(url), "https://api.telegram.org/bot%s/sendMessage", bot_token);
    
    // 构建JSON数据（Telegram API支持JSON格式）
    snprintf(json_data, 8192, "{\"chat_id\":\"%s\",\"text\":\"%s\"}", chat_id, escaped_text);
    
    result = send_http_post(url, json_data, response);
    if (result == 0) {
        printf("Telegram Response:\n%s\n", response);
    } else {
        printf("Telegram请求失败\n");
    }
    
    free(json_data);
    free(escaped_text);
    free(full_text);
    return result;
}

// ServerChan推送
int push_serverchan(const char *message, const char *sendkey, const char *title) {
    char url[512];
    char *json_data = malloc(8192);
    char *escaped_title = malloc(4096);
    char *escaped_msg = malloc(4096);
    char response[4096] = {0};
    int result;
    
    if (!json_data || !escaped_title || !escaped_msg) {
        if (json_data) free(json_data);
        if (escaped_title) free(escaped_title);
        if (escaped_msg) free(escaped_msg);
        return -1;
    }
    
    // 转义JSON字符串
    escape_json_string(title, escaped_title, 4096);
    escape_json_string(message, escaped_msg, 4096);
    
    // 构建URL: https://sctapi.ftqq.com/{SendKey}.send
    snprintf(url, sizeof(url), "https://sctapi.ftqq.com/%s.send", sendkey);
    
    // 构建JSON数据
    snprintf(json_data, 8192,
        "{\"title\":\"%s\",\"desp\":\"%s\"}",
        escaped_title, escaped_msg);
    
    result = send_http_post(url, json_data, response);
    if (result == 0) {
        printf("ServerChan Response:\n%s\n", response);
    } else {
        printf("ServerChan请求失败\n");
    }
    
    free(json_data);
    free(escaped_title);
    free(escaped_msg);
    return result;
}

// 获取短信存储数量（只获取数量，不获取具体内容）
int get_sms_storage_count(const char *resolved_port, int *storage_count) {
    if (!resolved_port || !storage_count) return -1;
    
    at_client_t client;
    if (at_client_open(&client, g_config.at_mode, resolved_port) != 0) {
        printf("初始化AT客户端失败。\n");
        return -1;
    }
    
    int result = 0;
    char *resp = NULL;
    
    // 只需要发送 AT+CPMS? 命令获取存储数量
    if (at_client_command(&client, "AT+CPMS?", &resp, 0) != 0 || !resp) {
        printf("读取存储状态失败。\n");
        result = -1;
        goto cleanup;
    }
    
    int cnt = 0;
    if (parse_cpms_count(resp, &cnt) == 0) {
        *storage_count = cnt;
    } else {
        *storage_count = -1;
        result = -1;
    }
    
cleanup:
    if (resp) {
        free(resp);
    }
    at_client_close(&client);
    return result;
}

// 获取短信数据（只获取具体内容，不获取存储数量）
int get_sms_data(sms_data_t *sms_data, const char *resolved_port) {
    if (!sms_data || !resolved_port) return -1;
    
    if (sms_data->messages) {
        free(sms_data->messages);
        sms_data->messages = NULL;
        sms_data->count = 0;
    }
    
    at_client_t client;
    if (at_client_open(&client, g_config.at_mode, resolved_port) != 0) {
        printf("初始化AT客户端失败。\n");
        return -1;
    }
    
    int result = 0;
    char *resp = NULL;
    
    if (at_client_command(&client, "AT+CMGF=0", &resp, 0) != 0 || !resp || !strstr(resp, "OK")) {
        printf("设置PDU模式失败。\n");
        result = -1;
        goto cleanup;
    }
    free(resp); resp = NULL;
    
    if (at_client_command(&client, "AT+CSCS=\"GSM\"", &resp, 0) != 0 || !resp || !strstr(resp, "OK")) {
        printf("设置字符集失败。\n");
        result = -1;
        goto cleanup;
    }
    free(resp); resp = NULL;
    
    // 获取短信列表
    if (at_client_command(&client, "AT+CMGL=4", &resp, 1) != 0 || !resp) {
        printf("读取短信列表失败。\n");
        result = -1;
        goto cleanup;
    }
    
    if (parse_cmgl_response(resp, sms_data) != 0) {
        printf("解析短信列表失败。\n");
        result = -1;
        goto cleanup;
    }
    
cleanup:
    if (resp) {
        free(resp);
    }
    at_client_close(&client);
    if (result != 0) {
        free_sms_data(sms_data);
    }
    return result;
}

// 验证短信数据
int is_sms_data_valid(const sms_data_t *sms_data, const sms_data_t *last_sms_data) {
    int i;
    
    if (!sms_data || sms_data->count == 0) {
        printf("短信数据为空，本轮跳过。\n");
        return 0;
    }
    
    // 检查每个短信字段
    for (i = 0; i < sms_data->count; i++) {
        if (sms_data->messages[i].index < 0 ||
            strlen(sms_data->messages[i].sender) == 0 ||
            strlen(sms_data->messages[i].content) == 0 ||
            sms_data->messages[i].timestamp == 0) {
            printf("短信字段缺失或为空！\n");
            return 0;
        }
    }
    
    return 1;
}

// 合并长短信分片
char* merge_multipart_sms(const sms_data_t *sms_data, int reference, char *sender, long *timestamp) {
    char *merged = malloc(2048);  // 合并后的内容
    if (!merged) return NULL;
    
    merged[0] = '\0';
    int found_parts = 0;
    *timestamp = 0;
    
    // 按part顺序合并
    for (int part_num = 1; part_num <= 10; part_num++) {  // 假设最多10片
        for (int i = 0; i < sms_data->count; i++) {
            if (sms_data->messages[i].reference == reference && 
                sms_data->messages[i].part == part_num) {
                
                // 记录第一片的时间戳和发件人
                if (part_num == 1) {
                    *timestamp = sms_data->messages[i].timestamp;
                    strncpy(sender, sms_data->messages[i].sender, 31);
                    sender[31] = '\0';
                }
                
                // 拼接内容
                strncat(merged, sms_data->messages[i].content, 
                       2047 - strlen(merged));
                found_parts++;
                break;
            }
        }
    }
    
    if (found_parts == 0) {
        free(merged);
        return NULL;
    }
    
    return merged;
}

// 合并所有长短信分片，返回合并后的短信数据
int merge_all_multipart_sms(const sms_data_t *input, sms_data_t *output) {
    if (!input || !output || input->count == 0) {
        output->messages = NULL;
        output->count = 0;
        return 0;
    }
    
    // 分配足够的内存（最多和输入一样多）
    output->messages = calloc(input->count, sizeof(sms_t));
    if (!output->messages) {
        output->count = 0;
        return -1;
    }
    
    int *processed = calloc(input->count, sizeof(int));
    if (!processed) {
        free(output->messages);
        output->messages = NULL;
        output->count = 0;
        return -1;
    }
    
    output->count = 0;
    
    for (int i = 0; i < input->count; i++) {
        if (processed[i]) continue;
        
        // 检查是否是长短信分片（有reference且total>1）
        if (input->messages[i].reference > 0 && input->messages[i].total > 1) {
            // 合并这个reference的所有分片
            char merged_content[2048] = {0};
            int found_parts = 0;
            int first_index = input->messages[i].index;
            long first_timestamp = input->messages[i].timestamp;
            char first_sender[32];
            strncpy(first_sender, input->messages[i].sender, sizeof(first_sender) - 1);
            first_sender[sizeof(first_sender) - 1] = '\0';
            
            // 按part顺序合并
            for (int part_num = 1; part_num <= input->messages[i].total; part_num++) {
                for (int j = 0; j < input->count; j++) {
                    if (processed[j]) continue;
                    if (input->messages[j].reference == input->messages[i].reference &&
                        input->messages[j].part == part_num) {
                        strncat(merged_content, input->messages[j].content, 
                               sizeof(merged_content) - strlen(merged_content) - 1);
                        processed[j] = 1;
                        found_parts++;
                        break;
                    }
                }
            }
            
            if (found_parts > 0) {
                // 添加合并后的短信
                output->messages[output->count].index = first_index;
                strncpy(output->messages[output->count].sender, first_sender, 
                       sizeof(output->messages[output->count].sender) - 1);
                output->messages[output->count].sender[sizeof(output->messages[output->count].sender) - 1] = '\0';
                output->messages[output->count].timestamp = first_timestamp;
                strncpy(output->messages[output->count].content, merged_content,
                       sizeof(output->messages[output->count].content) - 1);
                output->messages[output->count].content[sizeof(output->messages[output->count].content) - 1] = '\0';
                output->messages[output->count].reference = 0;
                output->messages[output->count].total = 1;
                output->messages[output->count].part = 1;
                output->count++;
            }
        } else {
            // 普通短信，直接复制
            output->messages[output->count] = input->messages[i];
            output->messages[output->count].reference = 0;
            output->messages[output->count].total = 1;
            output->messages[output->count].part = 1;
            output->count++;
            processed[i] = 1;
        }
    }
    
    free(processed);
    return 0;
}

// 格式化短信消息
void format_sms_message(const sms_t *sms, char *buffer, size_t buffer_size) {
    struct tm *tm_info;
    time_t timestamp = (time_t)sms->timestamp;
    char time_str[64];
    
    // timestamp已经是北京时间，直接转换即可
    tm_info = gmtime(&timestamp);  // 使用gmtime直接解析，不依赖系统时区
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // 检查是否是长短信
    if (sms->total > 1) {
        snprintf(buffer, buffer_size,
            "发件人: %s\n内容: %s\n时间: %s\n编号: %d (长短信 %d/%d)",
            sms->sender, sms->content, time_str, sms->index, sms->part, sms->total);
    } else {
        snprintf(buffer, buffer_size,
            "发件人: %s\n内容: %s\n时间: %s\n编号: %d",
            sms->sender, sms->content, time_str, sms->index);
    }
}

// 获取最新短信列表
sms_t* get_latest_sms_list(const sms_data_t *sms_data, int *count, int max_count) {
    sms_t *all_sorted;
    sms_t *result;
    int i, result_count;
    
    if (!sms_data || sms_data->count == 0) {
        *count = 0;
        return NULL;
    }
    
    // 先复制所有短信
    all_sorted = malloc(sms_data->count * sizeof(sms_t));
    if (!all_sorted) {
        *count = 0;
        return NULL;
    }
    
    for (i = 0; i < sms_data->count; i++) {
        all_sorted[i] = sms_data->messages[i];
    }
    
    // 对所有短信按时间戳排序（降序，最新的在前）
    qsort(all_sorted, sms_data->count, sizeof(sms_t), compare_sms_timestamp);
    
    // 取前max_count条（最新的）
    result_count = (sms_data->count < max_count) ? sms_data->count : max_count;
    result = malloc(result_count * sizeof(sms_t));
    if (!result) {
        free(all_sorted);
        *count = 0;
        return NULL;
    }
    
    for (i = 0; i < result_count; i++) {
        result[i] = all_sorted[i];
    }
    
    free(all_sorted);
    *count = result_count;
    return result;
}

// 获取最大时间戳
long get_max_timestamp(const sms_data_t *sms_data) {
    long max_ts = 0;
    int i;
    
    if (!sms_data || sms_data->count == 0) {
        return 0;
    }
    
    for (i = 0; i < sms_data->count; i++) {
        if (sms_data->messages[i].timestamp > max_ts) {
            max_ts = sms_data->messages[i].timestamp;
        }
    }
    
    return max_ts;
}

// 加载上次保存的短信数据
int load_last_sms_data(const char *filename, sms_data_t *sms_data) {
    FILE *file;
    char buffer[8192];
    int sms_count = 0;
    int i;
    
    sms_data->messages = NULL;
    sms_data->count = 0;
    
    file = fopen(filename, "r");
    if (!file) {
        return 0; // 文件不存在，正常情况
    }
    
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        fclose(file);
        return 0; // 文件为空
    }
    
    fclose(file);
    
    // 使用tiny-json解析保存的数据
    // 计算短信数量
    char *search_ptr = buffer;
    while ((search_ptr = strstr(search_ptr, "\"index\"")) != NULL) {
        sms_count++;
        search_ptr += 7; // 跳过 "index"
    }
    
    if (sms_count == 0) {
        return 0; // 没有短信数据
    }
    
    // 分配内存
    sms_data->messages = malloc(sms_count * sizeof(sms_t));
    if (!sms_data->messages) {
        return -1;
    }
    
    sms_data->count = 0;
    search_ptr = buffer;
    i = 0;
    
    // 解析每条短信
    while (i < sms_count && (search_ptr = strstr(search_ptr, "\"index\"")) != NULL) {
        char *sms_start = search_ptr;
        char *sms_end = strchr(sms_start, '}');
        
        if (sms_end) {
            char sms_json[1024];
            int sms_len = sms_end - sms_start + 1;
            if (sms_len > 0 && sms_len < sizeof(sms_json)) {
                strncpy(sms_json, sms_start, sms_len);
                sms_json[sms_len] = '\0';
                
                // 提取各个字段
                int index_val;
                char *sender_str = json_find_string(sms_json, "sender");
                char *timestamp_str = json_find_string(sms_json, "timestamp");
                char *content_str = json_find_string(sms_json, "content");
                
                // 使用json_find_int获取数字类型的index
                if (json_find_int(sms_json, "index", -1, &index_val) && sender_str && timestamp_str && content_str) {
                    sms_data->messages[sms_data->count].index = index_val;
                    
                    strncpy(sms_data->messages[sms_data->count].sender, sender_str, sizeof(sms_data->messages[sms_data->count].sender) - 1);
                    sms_data->messages[sms_data->count].sender[sizeof(sms_data->messages[sms_data->count].sender) - 1] = '\0';
                    
                    sms_data->messages[sms_data->count].timestamp = atol(timestamp_str);
                    
                    strncpy(sms_data->messages[sms_data->count].content, content_str, sizeof(sms_data->messages[sms_data->count].content) - 1);
                    sms_data->messages[sms_data->count].content[sizeof(sms_data->messages[sms_data->count].content) - 1] = '\0';
                    
                    sms_data->count++;
                }
                
                if (sender_str) free(sender_str);
                if (timestamp_str) free(timestamp_str);
                if (content_str) free(content_str);
            }
        }
        
        i++;
        search_ptr = sms_end + 1;
    }
    
    return sms_data->count > 0 ? 1 : 0;
}

// 保存短信数据
int save_last_sms_data(const char *filename, const sms_data_t *sms_data) {
    FILE *file;
    char *json_buffer = malloc(32768);  // 使用动态分配代替栈分配
    if (!json_buffer) {
        printf("JSON缓冲区内存分配失败\n");
        return -1;
    }
    size_t rem_len = 32768 - 1;
    char *dest = json_buffer;
    int i;
    
    // 先合并长短信分片
    sms_data_t merged_data = {NULL, 0};
    if (merge_all_multipart_sms(sms_data, &merged_data) != 0) {
        printf("合并长短信失败，使用原始数据\n");
        merged_data.messages = NULL;
        merged_data.count = 0;
    }
    
    const sms_data_t *data_to_save = (merged_data.count > 0) ? &merged_data : sms_data;
    
    file = fopen(filename, "w");
    if (!file) {
        printf("无法创建文件: %s\n", filename);
        free(json_buffer);
        if (merged_data.messages) {
            free(merged_data.messages);
        }
        return -1;
    }
    
    // 构建JSON结构
    dest = json_objOpen(dest, NULL, &rem_len);
    if (!dest) {
        fclose(file);
        free(json_buffer);
        if (merged_data.messages) {
            free(merged_data.messages);
        }
        return -1;
    }
    
    dest = json_arrOpen(dest, "msg", &rem_len);
    if (!dest) {
        fclose(file);
        free(json_buffer);
        if (merged_data.messages) {
            free(merged_data.messages);
        }
        return -1;
    }
    
    for (i = 0; i < data_to_save->count && rem_len > 100; i++) {
        dest = json_objOpen(dest, NULL, &rem_len);
        if (!dest) break;
        
        dest = json_int(dest, "index", data_to_save->messages[i].index, &rem_len);
        if (!dest) break;
        
        dest = json_nstr(dest, "sender", data_to_save->messages[i].sender, -1, &rem_len);
        if (!dest) break;
        
        dest = json_long(dest, "timestamp", data_to_save->messages[i].timestamp, &rem_len);
        if (!dest) break;
        
        dest = json_nstr(dest, "content", data_to_save->messages[i].content, -1, &rem_len);
        if (!dest) break;
        
        dest = json_objClose(dest, &rem_len);
        if (!dest) break;
    }
    
    dest = json_arrClose(dest, &rem_len);
    if (!dest) {
        fclose(file);
        free(json_buffer);
        if (merged_data.messages) {
            free(merged_data.messages);
        }
        return -1;
    }
    
    dest = json_end(dest, &rem_len);
    if (!dest) {
        fclose(file);
        free(json_buffer);
        if (merged_data.messages) {
            free(merged_data.messages);
        }
        return -1;
    }
    
    // json_end 只移除逗号，需要手动添加结尾的 }
    if (rem_len > 0) {
        *dest = '}';
        dest++;
        *dest = '\0';
        rem_len--;
    }
    
    fprintf(file, "%s", json_buffer);
    fclose(file);
    free(json_buffer);
    if (merged_data.messages) {
        free(merged_data.messages);
    }
    return 0;
}

// 写入摘要文件
void write_summary_to_file(int count, const char *content) {
    FILE *file;
    time_t now;
    struct tm *tm_info;
    char time_str[64];
    
    now = time(NULL);
    tm_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    file = fopen(g_summary_file, "a");
    if (file) {
        fprintf(file, "本次转发时间: %s\n", time_str);
        fprintf(file, "已完成: %d次短信转发\n", count);
        fprintf(file, "转发内容:\n\n%s\n\n", content);
        fclose(file);
    } else {
        printf("写入文件失败: %s\n", strerror(errno));
    }
}

// 释放短信数据内存
void free_sms_data(sms_data_t *sms_data) {
    if (sms_data && sms_data->messages) {
        free(sms_data->messages);
        sms_data->messages = NULL;
        sms_data->count = 0;
    }
}

// 比较短信时间戳（用于排序）
int compare_sms_timestamp(const void *a, const void *b) {
    const sms_t *sms_a = (const sms_t *)a;
    const sms_t *sms_b = (const sms_t *)b;
    
    if (sms_a->timestamp > sms_b->timestamp) return -1;
    if (sms_a->timestamp < sms_b->timestamp) return 1;
    return 0;
}

// 主转发函数
void forward(void) {
    sms_data_t current_sms_data = {NULL, 0};
    sms_data_t last_sms_data = {NULL, 0};
    sms_t *latest_sms = NULL;
    int latest_count = 0;
    long last_max_ts = 0, curr_max_ts = 0;
    int is_first_run = 1;
    int count = 0;
    int last_storage_count = 0;
    int i, j;
    // 使用动态分配减少栈占用
    char *msg_buffer = malloc(2048);
    char *msg_oneline = malloc(2048);
    
    if (!msg_buffer || !msg_oneline) {
        printf("内存分配失败\n");
        if (msg_buffer) free(msg_buffer);
        if (msg_oneline) free(msg_oneline);
        return;
    }
    FILE *lucimsg_file;
    
    if (!check_lock(g_lock_file)) {
        return;
    }
    
    // 读取配置
    if (read_config("/etc/config/smsforward", &g_config) < 0) {
        remove_lock(g_lock_file);
        return;
    }
    
    // 验证推送配置
    if (strcmp(g_config.push_type, "0") == 0) {
        if (strlen(g_config.pushplus_token) == 0) {
            printf("未填写PushPlus token，程序已退出！\n");
            remove_lock(g_lock_file);
            return;
        }
    } else if (strcmp(g_config.push_type, "1") == 0) {
        if (strlen(g_config.wxpusher_token) == 0 || strlen(g_config.wxpusher_topicids) == 0) {
            printf("未填写WxPusher token或topicids，程序已退出！\n");
            remove_lock(g_lock_file);
            return;
        }
    } else if (strcmp(g_config.push_type, "2") == 0) {
        if (strlen(g_config.pushdeer_key) == 0) {
            printf("未填写PushDeer key，程序已退出！\n");
            remove_lock(g_lock_file);
            return;
        }
    } else if (strcmp(g_config.push_type, "3") == 0) {
        if (strlen(g_config.wxpusher_webhook) == 0) {
            printf("未填写WxPusher Webhook，程序已退出！\n");
            remove_lock(g_lock_file);
            return;
        }
    } else if (strcmp(g_config.push_type, "4") == 0) {
        if (strlen(g_config.bark_did) == 0) {
            printf("未填写Bark did，程序已退出！\n");
            remove_lock(g_lock_file);
            return;
        }
    } else if (strcmp(g_config.push_type, "5") == 0) {
        if (strlen(g_config.dingtalk_token) == 0) {
            printf("未填写DingTalk token，程序已退出！\n");
            remove_lock(g_lock_file);
            return;
        }
    } else if (strcmp(g_config.push_type, "6") == 0) {
        if (strlen(g_config.feishu_webhook) == 0) {
            printf("未填写Feishu webhook，程序已退出！\n");
            remove_lock(g_lock_file);
            return;
        }
    } else if (strcmp(g_config.push_type, "7") == 0) {
        if (strlen(g_config.iyuu_token) == 0) {
            printf("未填写iyuu token，程序已退出！\n");
            remove_lock(g_lock_file);
            return;
        }
    } else if (strcmp(g_config.push_type, "8") == 0) {
        if (strlen(g_config.telegram_bot_token) == 0 || strlen(g_config.telegram_chat_id) == 0) {
            printf("未填写Telegram bot token或chat id，程序已退出！\n");
            remove_lock(g_lock_file);
            return;
        }
    } else if (strcmp(g_config.push_type, "9") == 0) {
        if (strlen(g_config.serverchan_sendkey) == 0) {
            printf("未填写ServerChan sendkey，程序已退出！\n");
            remove_lock(g_lock_file);
            return;
        }
    } else {
        printf("无效的推送类型，程序已退出！\n");
        remove_lock(g_lock_file);
        return;
    }
    
    // 加载上次保存的数据
    load_last_sms_data(g_record_file, &last_sms_data);
    if (last_sms_data.count > 0) {
        latest_sms = get_latest_sms_list(&last_sms_data, &latest_count, 20);
        if (latest_sms) {
            last_max_ts = get_max_timestamp(&last_sms_data);
            free(latest_sms);
        }
    }
    last_storage_count = last_sms_data.count;
    
    printf("Enjoy! 已完成初始化并开启转发功能，重启后完成开机自启。\n");
    printf("上次保存的最新短信时间戳: %ld\n", last_max_ts);
    
    const char *push_type_names[] = {
        "PushPlus", "WxPusher", "PushDeer", "WxWork Webhook",
        "Bark", "DingTalk", "Feishu", "iyuu", "Telegram", "ServerChan"
    };
    int push_type_index = atoi(g_config.push_type);
    if (push_type_index >= 0 && push_type_index < 10) {
        printf("推送方式: %s\n", push_type_names[push_type_index]);
    } else {
        printf("推送方式: 未知\n");
    }
    
    // 在循环外解析串口配置，避免每次循环都重新解析
    char resolved_port[64];
    if (resolve_at_port(resolved_port, sizeof(resolved_port)) != 0) {
        printf("无法解析串口配置，程序退出。\n");
        remove_lock(g_lock_file);
        free(msg_buffer);
        free(msg_oneline);
        free_sms_data(&last_sms_data);
        return;
    }
    printf("使用串口: %s (模式: %s)\n", resolved_port, g_config.at_mode);
    
    while (1) {
        // 先获取短信存储数量（快速检查）
        int storage_count = -1;
        if (get_sms_storage_count(resolved_port, &storage_count) != 0) {
            printf("获取存储数量失败，跳过本轮。\n");
            sleep(13);
            continue;
        }
        
        // 检查数量是否变化
        int storage_changed = (storage_count != last_storage_count);
        if (storage_changed) {
            printf("短信存储数量变化: 上次=%d, 当前=%d，开始获取短信内容\n", last_storage_count, storage_count);
            
            // 数量变化，获取具体短信内容
            if (get_sms_data(&current_sms_data, resolved_port) != 0) {
                printf("获取短信数据失败，跳过本轮。\n");
                sleep(13);
                continue;
            }
            
            if (!is_sms_data_valid(&current_sms_data, &last_sms_data)) {
                printf("AT数据异常，跳过本轮。\n");
                free_sms_data(&current_sms_data);
                sleep(13);
                continue;
            }
        } else {
            // 数量没有变化，跳过获取短信内容，直接进入下一轮
            // printf("短信存储数量未变化: %d，跳过获取短信内容\n", storage_count);
            sleep(13);
            continue;
        }
        
        // 获取最新短信列表
        if (latest_sms) {
            free(latest_sms);
        }
        latest_sms = get_latest_sms_list(&current_sms_data, &latest_count, 20);
        if (!latest_sms) {
            free_sms_data(&current_sms_data);
            sleep(13);
            continue;
        }
        
        curr_max_ts = get_max_timestamp(&current_sms_data);
        
        if (is_first_run) {
            printf("程序第一次运行，保存当前短信数据，不进行转发。\n");
            save_last_sms_data(g_record_file, &current_sms_data);
            last_max_ts = curr_max_ts;
            last_storage_count = storage_count;
            is_first_run = 0;
        } else {
            // 判断是否有新短信
            if (curr_max_ts > last_max_ts) {
                printf("检测到新短信！\n");
                
                // 找出所有大于last_max_ts的短信
                int processed_refs[20] = {0};  // 记录已处理的长短信reference
                int processed_count = 0;
                
                for (i = 0; i < latest_count; i++) {
                    if (latest_sms[i].timestamp > last_max_ts) {
                        // 检查是否是长短信
                        if (latest_sms[i].total > 1 && latest_sms[i].reference > 0) {
                            // 检查这个reference是否已经处理过
                            int already_processed = 0;
                            for (int k = 0; k < processed_count; k++) {
                                if (processed_refs[k] == latest_sms[i].reference) {
                                    already_processed = 1;
                                    printf("跳过长短信分片 (ref=%d, part=%d/%d) - 已处理过\n",
                                           latest_sms[i].reference, latest_sms[i].part, latest_sms[i].total);
                                    break;
                                }
                            }
                            
                            if (already_processed) {
                                continue;  // 跳过这一片
                            }
                            
                            // 记录这个reference
                            if (processed_count < 50) {
                                processed_refs[processed_count++] = latest_sms[i].reference;
                            }
                            
                            // 是长短信，尝试合并
                            char sender[32];
                            long merged_ts;
                            char *merged_content = merge_multipart_sms(&current_sms_data, 
                                                                       latest_sms[i].reference,
                                                                       sender, &merged_ts);
                            if (merged_content) {
                                // 使用合并后的内容
                                struct tm *tm_info;
                                time_t timestamp = (time_t)merged_ts;
                                char time_str[64];
                                
                                // timestamp已经是北京时间，直接转换即可
                                tm_info = gmtime(&timestamp);
                                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
                                
                                snprintf(msg_buffer, 2048,
                                    "发件人: %s\n内容: %s\n时间: %s\n编号: %d",
                                    sender, merged_content, time_str, latest_sms[i].index);
                                free(merged_content);
                            } else {
                                // 合并失败，使用原始内容
                                format_sms_message(&latest_sms[i], msg_buffer, 2048);
                            }
                        } else {
                            // 普通短信
                            format_sms_message(&latest_sms[i], msg_buffer, 2048);
                        }
                        
                        // 创建单行消息
                        strcpy(msg_oneline, msg_buffer);
                        for (j = 0; msg_oneline[j]; j++) {
                            if (msg_oneline[j] == '\n' || msg_oneline[j] == '\r') {
                                msg_oneline[j] = ' ';
                            }
                        }
                        
                        // 写入lucimsg.file
                        lucimsg_file = fopen("/tmp/lucimsg.file", "w");
                        if (lucimsg_file) {
                            fprintf(lucimsg_file, "来短信了，短信内容：%s\n6\n#81D3FF", msg_oneline);
                            fclose(lucimsg_file);
                        } else {
                            printf("写入 /tmp/lucimsg.file 失败: %s\n", strerror(errno));
                        }
                        
                        // 根据推送类型选择推送方式
                        if (strcmp(g_config.push_type, "0") == 0) {
                            push_pushplus(msg_buffer, g_config.pushplus_token, g_config.sms_title);
                        } else if (strcmp(g_config.push_type, "1") == 0) {
                            push_wxpusher(msg_buffer, g_config.wxpusher_token, g_config.wxpusher_topicids, g_config.sms_title);
                        } else if (strcmp(g_config.push_type, "2") == 0) {
                            push_pushdeer(msg_buffer, g_config.pushdeer_key, g_config.sms_title);
                        } else if (strcmp(g_config.push_type, "3") == 0) {
                            push_wxpusher_webhook(msg_buffer, g_config.wxpusher_webhook, g_config.sms_title);
                        } else if (strcmp(g_config.push_type, "4") == 0) {
                            push_bark(msg_buffer, g_config.bark_did, g_config.sms_title);
                        } else if (strcmp(g_config.push_type, "5") == 0) {
                            push_dingtalk(msg_buffer, g_config.dingtalk_token, g_config.sms_title);
                        } else if (strcmp(g_config.push_type, "6") == 0) {
                            push_feishu(msg_buffer, g_config.feishu_webhook, g_config.sms_title);
                        } else if (strcmp(g_config.push_type, "7") == 0) {
                            push_iyuu(msg_buffer, g_config.iyuu_token, g_config.sms_title);
                        } else if (strcmp(g_config.push_type, "8") == 0) {
                            push_telegram(msg_buffer, g_config.telegram_bot_token, g_config.telegram_chat_id, g_config.sms_title);
                        } else if (strcmp(g_config.push_type, "9") == 0) {
                            push_serverchan(msg_buffer, g_config.serverchan_sendkey, g_config.sms_title);
                        }
                        
                        count++;
                        write_summary_to_file(count, msg_buffer);
                    }
                }
                
                // 更新保存
                save_last_sms_data(g_record_file, &current_sms_data);
                last_max_ts = curr_max_ts;
                last_storage_count = storage_count;
            } else {
                if (storage_changed) {
                    printf("短信数量变化但无新内容，更新缓存。\n");
                    save_last_sms_data(g_record_file, &current_sms_data);
                    last_max_ts = curr_max_ts;
                    last_storage_count = storage_count;
                } else {
                    printf("没有新短信。\n");
                }
            }
        }
        
        // 清理内存
        free_sms_data(&current_sms_data);
        sleep(13);
    }
    
    // 清理资源
    if (latest_sms) {
        free(latest_sms);
    }
    free_sms_data(&last_sms_data);
    free(msg_buffer);
    free(msg_oneline);
    remove_lock(g_lock_file);
}

// 备用解析函数 - 使用简单的字符串搜索
int parse_sms_fallback(const char *buffer, sms_data_t *sms_data) {
    char temp_buffer[16384];
    char *search_ptr;
    int sms_count = 0;
    int i;
    
    // 复制缓冲区内容
    strncpy(temp_buffer, buffer, sizeof(temp_buffer) - 1);
    temp_buffer[sizeof(temp_buffer) - 1] = '\0';
    
    // 简单计算短信数量（通过统计 "index" 字符串）
    search_ptr = temp_buffer;
    while ((search_ptr = strstr(search_ptr, "\"index\"")) != NULL) {
        sms_count++;
        search_ptr += 7; // 跳过 "index" 字符串
    }
    
    if (sms_count == 0) {
        return -1;
    }
    
    // 分配内存
    sms_data->messages = malloc(sms_count * sizeof(sms_t));
    if (!sms_data->messages) {
        printf("内存分配失败\n");
        return -1;
    }
    
    // 初始化所有短信数据
    for (i = 0; i < sms_count; i++) {
        sms_data->messages[i].index = 0;
        strcpy(sms_data->messages[i].sender, "");
        sms_data->messages[i].timestamp = 0;
        strcpy(sms_data->messages[i].content, "");
    }
    
    sms_data->count = sms_count;
    
    // 简单的字符串提取
    search_ptr = temp_buffer;
    i = 0;
    
    while (i < sms_count && (search_ptr = strstr(search_ptr, "\"index\"")) != NULL) {
        char *line_start = search_ptr;
        char *line_end = strchr(line_start, '}');
        
        if (line_end) {
            char line[1024];
            int line_len = line_end - line_start + 1;
            if (line_len > 0 && line_len < sizeof(line)) {
                strncpy(line, line_start, line_len);
                line[line_len] = '\0';
                
                // 提取index
                char *index_start = strstr(line, "\"index\"");
                if (index_start) {
                    char *index_value = strchr(index_start, ':');
                    if (index_value) {
                        index_value++;
                        while (*index_value == ' ' || *index_value == '\t') index_value++;
                        if (*index_value >= '0' && *index_value <= '9') {
                            char index_str[32] = {0};
                            int j = 0;
                            while (*index_value >= '0' && *index_value <= '9' && j < 31) {
                                index_str[j++] = *index_value++;
                            }
                            sms_data->messages[i].index = atoi(index_str);
                        }
                    }
                }
                
                // 提取sender
                char *sender_start = strstr(line, "\"sender\"");
                if (sender_start) {
                    char *sender_value = strchr(sender_start, '"');
                    if (sender_value) {
                        sender_value = strchr(sender_value + 1, '"');
                        if (sender_value) {
                            sender_value++;
                            char *sender_end = strchr(sender_value, '"');
                            if (sender_end) {
                                int len = sender_end - sender_value;
                                if (len > 0 && len < sizeof(sms_data->messages[i].sender)) {
                                    strncpy(sms_data->messages[i].sender, sender_value, len);
                                    sms_data->messages[i].sender[len] = '\0';
                                }
                            }
                        }
                    }
                }
                
                // 提取timestamp
                char *timestamp_start = strstr(line, "\"timestamp\"");
                if (timestamp_start) {
                    char *timestamp_value = strchr(timestamp_start, ':');
                    if (timestamp_value) {
                        timestamp_value++;
                        while (*timestamp_value == ' ' || *timestamp_value == '\t') timestamp_value++;
                        if (*timestamp_value >= '0' && *timestamp_value <= '9') {
                            char timestamp_str[32] = {0};
                            int j = 0;
                            while (*timestamp_value >= '0' && *timestamp_value <= '9' && j < 31) {
                                timestamp_str[j++] = *timestamp_value++;
                            }
                            sms_data->messages[i].timestamp = atol(timestamp_str);
                        }
                    }
                }
                
                // 提取content
                char *content_start = strstr(line, "\"content\"");
                if (content_start) {
                    char *content_value = strchr(content_start, '"');
                    if (content_value) {
                        content_value = strchr(content_value + 1, '"');
                        if (content_value) {
                            content_value++;
                            char *content_end = strchr(content_value, '"');
                            if (content_end) {
                                int len = content_end - content_value;
                                if (len > 0 && len < sizeof(sms_data->messages[i].content)) {
                                    strncpy(sms_data->messages[i].content, content_value, len);
                                    sms_data->messages[i].content[len] = '\0';
                                }
                            }
                        }
                    }
                }
            }
        }
        
        i++;
        search_ptr = line_end + 1;
    }
    
    return 0;
}

// 测试推送函数
int test_push(void) {
    const char *test_message = "这是一条测试短信\n\n发送时间: 测试推送功能\n发送者: 系统测试\n内容: 如果您收到这条消息，说明推送配置正确！";
    
    // 读取配置
    if (read_config("/etc/config/smsforward", &g_config) < 0) {
        printf("读取配置文件失败\n");
        return -1;
    }
    
    // 验证推送配置
    if (strcmp(g_config.push_type, "0") == 0) {
        if (strlen(g_config.pushplus_token) == 0) {
            printf("未填写PushPlus token\n");
            return -1;
        }
        printf("使用PushPlus推送测试消息...\n");
        return push_pushplus(test_message, g_config.pushplus_token, g_config.sms_title);
    } else if (strcmp(g_config.push_type, "1") == 0) {
        if (strlen(g_config.wxpusher_token) == 0 || strlen(g_config.wxpusher_topicids) == 0) {
            printf("未填写WxPusher token或topicids\n");
            return -1;
        }
        printf("使用WxPusher推送测试消息...\n");
        return push_wxpusher(test_message, g_config.wxpusher_token, g_config.wxpusher_topicids, g_config.sms_title);
    } else if (strcmp(g_config.push_type, "2") == 0) {
        if (strlen(g_config.pushdeer_key) == 0) {
            printf("未填写PushDeer key\n");
            return -1;
        }
        printf("使用PushDeer推送测试消息...\n");
        return push_pushdeer(test_message, g_config.pushdeer_key, g_config.sms_title);
    } else if (strcmp(g_config.push_type, "3") == 0) {
        if (strlen(g_config.wxpusher_webhook) == 0) {
            printf("未填写WxPusher Webhook\n");
            return -1;
        }
        printf("使用WxWork Webhook推送测试消息...\n");
        return push_wxpusher_webhook(test_message, g_config.wxpusher_webhook, g_config.sms_title);
    } else if (strcmp(g_config.push_type, "4") == 0) {
        if (strlen(g_config.bark_did) == 0) {
            printf("未填写Bark did\n");
            return -1;
        }
        printf("使用Bark推送测试消息...\n");
        return push_bark(test_message, g_config.bark_did, g_config.sms_title);
    } else if (strcmp(g_config.push_type, "5") == 0) {
        if (strlen(g_config.dingtalk_token) == 0) {
            printf("未填写DingTalk token\n");
            return -1;
        }
        printf("使用DingTalk推送测试消息...\n");
        return push_dingtalk(test_message, g_config.dingtalk_token, g_config.sms_title);
    } else if (strcmp(g_config.push_type, "6") == 0) {
        if (strlen(g_config.feishu_webhook) == 0) {
            printf("未填写Feishu webhook\n");
            return -1;
        }
        printf("使用Feishu推送测试消息...\n");
        return push_feishu(test_message, g_config.feishu_webhook, g_config.sms_title);
    } else if (strcmp(g_config.push_type, "7") == 0) {
        if (strlen(g_config.iyuu_token) == 0) {
            printf("未填写iyuu token\n");
            return -1;
        }
        printf("使用IYUU推送测试消息...\n");
        return push_iyuu(test_message, g_config.iyuu_token, g_config.sms_title);
    } else if (strcmp(g_config.push_type, "8") == 0) {
        if (strlen(g_config.telegram_bot_token) == 0 || strlen(g_config.telegram_chat_id) == 0) {
            printf("未填写Telegram bot token或chat id\n");
            return -1;
        }
        printf("使用Telegram推送测试消息...\n");
        return push_telegram(test_message, g_config.telegram_bot_token, g_config.telegram_chat_id, g_config.sms_title);
    } else if (strcmp(g_config.push_type, "9") == 0) {
        if (strlen(g_config.serverchan_sendkey) == 0) {
            printf("未填写ServerChan sendkey\n");
            return -1;
        }
        printf("使用ServerChan推送测试消息...\n");
        return push_serverchan(test_message, g_config.serverchan_sendkey, g_config.sms_title);
    } else {
        printf("无效的推送类型\n");
        return -1;
    }
}

int main(int argc, char *argv[]) {
    // 处理 -t 参数（测试推送）
    if (argc > 1 && strcmp(argv[1], "-t") == 0) {
        return test_push();
    }
    
    // 正常模式，运行转发功能
    forward();
    return 0;
}
