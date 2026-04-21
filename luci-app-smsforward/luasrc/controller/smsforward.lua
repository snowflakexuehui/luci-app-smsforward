module("luci.controller.smsforward", package.seeall)

function index()
    entry({"admin", "modem", "smsforward"}, call("action_form"), _( "SmsForward"), 90).dependent = true
    entry({"admin", "modem", "smsforward_status"}, call("action_status")).leaf = true
    entry({"admin", "modem", "smsforward_history"}, call("action_history")).leaf = true
    entry({"admin", "modem", "smsforward_clear_history"}, call("action_clear_history")).leaf = true
    entry({"admin", "modem", "smsforward_test"}, call("action_test_push")).leaf = true
end

local uci = require "luci.model.uci".cursor()
local sys = require "luci.sys"

function action_status()
    local running = sys.call("pgrep -f /usr/bin/smsforward >/dev/null") == 0
    luci.http.prepare_content("application/json")
    luci.http.write_json({ running = running })
end

function action_history()
    local file = "/tmp/smsforwardsum.conf"
    local content = ""
    local f = io.open(file, "rb")
    if f then
        content = f:read("*a") or ""
        f:close()
        -- 尝试转为utf-8（OpenWrt一般默认utf-8，保险起见）
        if content ~= "" and not content:find("^[%z\1-\127\194-\244]*$") then
            content = require("luci.sys").exec("cat "..file)
        end
    end
    luci.http.prepare_content("text/plain; charset=utf-8")
    luci.http.write(content)
end

function action_clear_history()
    local file = "/tmp/smsforwardsum.conf"
    local ok = true
    if os.remove(file) == nil then
        ok = not (io.open(file, "r") ~= nil)
    end
    luci.http.prepare_content("application/json")
    luci.http.write_json({ success = ok })
end

function action_test_push()
    local result = sys.call("/usr/bin/smsforward -t >/tmp/smsforward_test.log 2>&1")
    local output = ""
    local f = io.open("/tmp/smsforward_test.log", "r")
    if f then
        output = f:read("*a") or ""
        f:close()
    end
    luci.http.prepare_content("application/json")
    luci.http.write_json({ 
        success = (result == 0),
        output = output
    })
end

function action_form()
    local uci = require "luci.model.uci".cursor()
    local http = require "luci.http"
    local util = require "luci.util"
    local translate = luci.i18n.translate
    local tpl = require "luci.template"
    local config = {}
    local token_error = false
    uci:foreach("smsforward", "smsforward", function(s)
        config = s
    end)
    if http.formvalue("cbi.apply") then
        local enable = http.formvalue("enable")
        if enable ~= "1" then enable = "0" end
        uci:set("smsforward", config[".name"], "enable", enable)
        
        -- 处理 push_type
        local push_type = http.formvalue("push_type") or "0"
        uci:set("smsforward", config[".name"], "push_type", push_type)
        
        -- 处理 pushplus_token
        local pushplus_token = http.formvalue("pushplus_token") or ""
        local is_token_obfuscated = pushplus_token:match("^....%*+....$")
        if pushplus_token == "" and enable == "1" and push_type == "0" then
            token_error = true
        elseif not is_token_obfuscated and pushplus_token ~= "" then
            uci:set("smsforward", config[".name"], "pushplus_token", pushplus_token)
        end
        
        -- 处理 wxpusher_token
        local wxpusher_token = http.formvalue("wxpusher_token") or ""
        local is_wxpusher_token_obfuscated = wxpusher_token:match("^....%*+....$")
        if wxpusher_token == "" and enable == "1" and push_type == "1" then
            token_error = true
        elseif not is_wxpusher_token_obfuscated and wxpusher_token ~= "" then
            uci:set("smsforward", config[".name"], "wxpusher_token", wxpusher_token)
        end
        
        -- 处理 wxpusher_topicids
        local wxpusher_topicids = http.formvalue("wxpusher_topicids") or ""
        if wxpusher_topicids == "" and enable == "1" and push_type == "1" then
            token_error = true
        elseif wxpusher_topicids ~= "" then
            uci:set("smsforward", config[".name"], "wxpusher_topicids", wxpusher_topicids)
        end
        
        -- 处理 sms_title
        local sms_title = http.formvalue("sms_title") or ""
        if sms_title == "" then
            sms_title = "新信息"
        end
        uci:set("smsforward", config[".name"], "sms_title", sms_title)

        -- 处理 pushdeer_key
        local pushdeer_key = http.formvalue("pushdeer_key") or ""
        local is_pushdeer_key_obfuscated = pushdeer_key:match("^....%*+....$")
        if pushdeer_key == "" and enable == "1" and push_type == "2" then
            token_error = true
        elseif not is_pushdeer_key_obfuscated and pushdeer_key ~= "" then
            uci:set("smsforward", config[".name"], "pushdeer_key", pushdeer_key)
        end

        -- 处理 wxpusher_webhook
        local wxpusher_webhook = http.formvalue("wxpusher_webhook") or ""
        local is_wxpusher_webhook_obfuscated = wxpusher_webhook:match("^....%*+....$")
        if wxpusher_webhook == "" and enable == "1" and push_type == "3" then
            token_error = true
        elseif not is_wxpusher_webhook_obfuscated and wxpusher_webhook ~= "" then
            uci:set("smsforward", config[".name"], "wxpusher_webhook", wxpusher_webhook)
        end

        -- 处理 bark_did
        local bark_did = http.formvalue("bark_did") or ""
        local is_bark_did_obfuscated = bark_did:match("^....%*+....$")
        if bark_did == "" and enable == "1" and push_type == "4" then
            token_error = true
        elseif not is_bark_did_obfuscated and bark_did ~= "" then
            uci:set("smsforward", config[".name"], "bark_did", bark_did)
        end

        -- 处理 dingtalk_token
        local dingtalk_token = http.formvalue("dingtalk_token") or ""
        local is_dingtalk_token_obfuscated = dingtalk_token:match("^....%*+....$")
        if dingtalk_token == "" and enable == "1" and push_type == "5" then
            token_error = true
        elseif not is_dingtalk_token_obfuscated and dingtalk_token ~= "" then
            uci:set("smsforward", config[".name"], "dingtalk_token", dingtalk_token)
        end

        -- 处理 feishu_webhook
        local feishu_webhook = http.formvalue("feishu_webhook") or ""
        local is_feishu_webhook_obfuscated = feishu_webhook:match("^....%*+....$")
        if feishu_webhook == "" and enable == "1" and push_type == "6" then
            token_error = true
        elseif not is_feishu_webhook_obfuscated and feishu_webhook ~= "" then
            uci:set("smsforward", config[".name"], "feishu_webhook", feishu_webhook)
        end

        -- 处理 iyuu_token
        local iyuu_token = http.formvalue("iyuu_token") or ""
        local is_iyuu_token_obfuscated = iyuu_token:match("^....%*+....$")
        if iyuu_token == "" and enable == "1" and push_type == "7" then
            token_error = true
        elseif not is_iyuu_token_obfuscated and iyuu_token ~= "" then
            uci:set("smsforward", config[".name"], "iyuu_token", iyuu_token)
        end

        -- 处理 telegram_bot_token
        local telegram_bot_token = http.formvalue("telegram_bot_token") or ""
        local is_telegram_bot_token_obfuscated = telegram_bot_token:match("^....%*+....$")
        if telegram_bot_token == "" and enable == "1" and push_type == "8" then
            token_error = true
        elseif not is_telegram_bot_token_obfuscated and telegram_bot_token ~= "" then
            uci:set("smsforward", config[".name"], "telegram_bot_token", telegram_bot_token)
        end

        -- 处理 telegram_chat_id
        local telegram_chat_id = http.formvalue("telegram_chat_id") or ""
        if telegram_chat_id == "" and enable == "1" and push_type == "8" then
            token_error = true
        elseif telegram_chat_id ~= "" then
            uci:set("smsforward", config[".name"], "telegram_chat_id", telegram_chat_id)
        end

        -- 处理 serverchan_sendkey
        local serverchan_sendkey = http.formvalue("serverchan_sendkey") or ""
        local is_serverchan_sendkey_obfuscated = serverchan_sendkey:match("^....%*+....$")
        if serverchan_sendkey == "" and enable == "1" and push_type == "9" then
            token_error = true
        elseif not is_serverchan_sendkey_obfuscated and serverchan_sendkey ~= "" then
            uci:set("smsforward", config[".name"], "serverchan_sendkey", serverchan_sendkey)
        end

        uci:commit("smsforward")
        if not token_error then
            if enable == "1" then
                -- 检查当前服务是否已运行，已运行则先停止再启动
                if sys.call("pgrep -f /usr/bin/smsforward >/dev/null") == 0 then
                    os.execute("/etc/init.d/smsforward stop")
                end
                os.execute("/etc/init.d/smsforward start")
            else
                os.execute("/etc/init.d/smsforward stop")
            end
            http.redirect(luci.dispatcher.build_url("admin/modem/smsforward"))
            return
        end
    end
    tpl.render("smsforward/smsforward", {
        enable = config.enable or "0",
        push_type = config.push_type or "0",
        pushplus_token = config.pushplus_token or "",
        wxpusher_token = config.wxpusher_token or "",
        wxpusher_topicids = config.wxpusher_topicids or "",
        sms_title = config.sms_title or "",
        translate = translate,
        token_error = token_error,
        text_on = translate("On"),
        text_off = translate("Off"),
        pushdeer_key = config.pushdeer_key or "",
        wxpusher_webhook = config.wxpusher_webhook or "",
        bark_did = config.bark_did or "",
        dingtalk_token = config.dingtalk_token or "",
        feishu_webhook = config.feishu_webhook or "",
        iyuu_token = config.iyuu_token or "",
        telegram_bot_token = config.telegram_bot_token or "",
        telegram_chat_id = config.telegram_chat_id or "",
        serverchan_sendkey = config.serverchan_sendkey or ""
    })
end 