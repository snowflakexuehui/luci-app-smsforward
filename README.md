# OpenWRT短信转发服务

一个功能强大的OpenWRT短信转发服务，能够自动将接收到的短信转发到多种推送通知服务。

## 功能特性

- **多种推送服务支持**：
  - 推送加 (PushPlus)
  - 微信推送 (WxPusher)
  - PushDeer
  - 企业微信 Webhook
  - Bark
  - 钉钉 (DingTalk)
  - 飞书 (Feishu)
  - iyuu
  - Telegram 机器人
  - Server酱

- **高级功能**：
  - 实时短信监控和转发
  - JSON配置支持
  - 文件锁定机制确保数据完整性
  - 短信数据验证和过滤
  - 持久化数据存储和历史记录
  - AT命令接口与调制解调器通信
  - 基于LuCI的Web配置界面
  - 测试推送功能

## 安装方法

### 前提条件

- OpenWRT系统
- 兼容的GSM/3G/4G调制解调器
- 已安装LuCI Web界面

### 安装步骤

1. **安装软件包**：
   ```bash
   opkg update
   opkg install smsforward luci-app-smsforward
   ```

2. **通过LuCI配置**：
   - 访问OpenWRT Web界面
   - 导航至：`调制解调器` → `短信转发`
   - 配置您喜欢的推送服务
   - 输入所需的令牌/密钥
   - 启用服务

3. **手动配置**（可选）：
   直接编辑 `/etc/config/smsforward`：
   ```
   config smsforward 'smsforward'
       option enable '1'
       option push_type '0'
       option pushplus_token '你的令牌'
       option sms_title '短信提醒'
   ```

## 配置选项

### 推送服务类型

| 类型 | 服务名称 | 必需配置 |
|------|----------|----------|
| 0 | 推送加 | `pushplus_token` |
| 1 | 微信推送 | `wxpusher_token`, `wxpusher_topicids` |
| 2 | PushDeer | `pushdeer_key` |
| 3 | 企业微信 | `wxpusher_webhook` |
| 4 | Bark | `bark_did` |
| 5 | 钉钉 | `dingtalk_token` |
| 6 | 飞书 | `feishu_webhook` |
| 7 | iyuu | `iyuu_token` |
| 8 | Telegram | `telegram_bot_token`, `telegram_chat_id` |
| 9 | Server酱 | `serverchan_sendkey` |

### AT端口配置

- 默认：`qmodem`（自动检测调制解调器端口）
- 自定义：指定调制解调器的AT端口（例如：`/dev/ttyUSB2`）

## 使用方法

### Web界面

通过LuCI访问短信转发配置：
- **状态**：检查服务是否运行
- **历史记录**：查看短信转发历史
- **测试推送**：发送测试通知
- **清除历史**：清除转发历史记录

### 命令行

检查服务状态：
```bash
/etc/init.d/smsforward status
```

启动/停止服务：
```bash
/etc/init.d/smsforward start
/etc/init.d/smsforward stop
/etc/init.d/smsforward restart
```

手动测试：
```bash
/usr/bin/smsforward -t
```

## 文件位置

- **主程序**：`/usr/bin/smsforward`
- **启动脚本**：`/etc/init.d/smsforward`
- **配置文件**：`/etc/config/smsforward`
- **锁文件**：`/tmp/smsforward.lock`
- **数据存储**：`/tmp/last_sms_data.txt`
- **历史文件**：`/tmp/smsforwardsum.conf`
- **LuCI控制器**：`/usr/lib/lua/luci/controller/smsforward.lua`
- **LuCI视图**：`/usr/lib/lua/luci/view/smsforward/`

## 架构设计

应用程序由两个主要组件组成：

1. **后端服务** (`smsforward`)：
   - 使用C语言编写，性能和资源占用优化
   - 监控调制解调器AT端口接收短信
   - 解析短信数据并验证内容
   - 通过配置的推送服务发送通知
   - 实现文件锁定确保数据完整性

2. **Web界面** (`luci-app-smsforward`)：
   - 基于LuCI的Web配置界面
   - 实时状态监控
   - 配置管理
   - 测试功能
   - 历史记录查看

## 开发指南

### 从源码编译

1. **设置OpenWRT SDK**
2. **编译软件包**：
   ```bash
   make package/smsforward/compile
   make package/luci-app-smsforward/compile
   ```

### 项目结构

```
luci-app-smsforward/
├── luci-app-smsforward/          # LuCI Web界面
│   ├── luasrc/controller/        # LuCI控制器
│   ├── htdocs/                   # Web资源
│   ├── po/                       # 翻译文件
│   └── root/                     # 启动脚本
└── smsforward/                   # 后端服务
    ├── smsforward.c              # 主程序
    ├── smsforward.h              # 头文件
    ├── json.c/json.h             # JSON解析
    ├── serial.c/serial.h         # 串口通信
    └── Makefile                  # 编译配置
```

## 故障排除

### 常见问题

1. **服务无法启动**：
   - 检查调制解调器连接
   - 验证AT端口配置
   - 查看系统日志：`logread | grep smsforward`

2. **推送通知不工作**：
   - 验证令牌/密钥配置
   - 使用Web界面的测试按钮测试
   - 检查网络连接

3. **权限问题**：
   - 确保调制解调器设备有正确的权限
   - 检查`/tmp`目录权限

### 调试模式

通过查看系统日志启用调试：
```bash
logread | grep smsforward
```

## 贡献指南

1. Fork 仓库
2. 创建功能分支
3. 进行修改
4. 彻底测试
5. 提交拉取请求

## 许可证

MIT许可证 - 详见LICENSE文件。

## 作者

由 Snowflakes (xuehui1994@outlook.com) 开发

## 技术支持

如有问题或功能需求，请使用GitHub问题跟踪器。

---

**注意**：此应用程序需要连接到OpenWRT设备的兼容GSM/3G/4G调制解调器才能正常工作。