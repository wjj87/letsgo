# 网吧上网计费系统

这是一个基于 C 语言后台、Python 封装服务和 Web 前端的网吧上网计费系统。


## 项目结构

- `netbar_billing.c`：核心 C 程序，包含用户注册、登录、充值、上机、下机、计费、消费记录等功能。
- `wrapper_server.py`：Python HTTP 服务，将 C 程序封装成 REST API，前端通过 API 调用后端功能。
- `index.html`：Web 前端页面，提供用户登录、注册、上机、充值、查看历史以及管理员管理界面。
- `app.js`：前端交互逻辑，用于与封装服务通信并渲染界面。
- `run_wrapper.bat`：Windows 一键启动脚本，负责编译 C 程序（若未编译）并启动 Python 服务。
- `users.dat` / `records.dat` / `pcs.dat`：运行时生成的持久化数据文件。

## 功能概述
- 用户注册与登录
- 余额充值
- 选择空闲电脑上机
- 实时计费、查询当前费用
- 结束上机并生成消费记录
- 用户查看历史消费记录
- 管理员登录后查看用户列表、消费记录、统计信息
- 管理员强制下线指定用户

## 运行要求
- Windows 或类 Unix 环境
- Python 3
- GCC（用于编译 `netbar_billing.c`，例如 MinGW）

## 快速启动
1. 打开命令行终端，进入项目目录。
2. 运行：
```batch
run_wrapper.bat
```

该脚本会：
- 如果不存在 `netbar_billing.exe`，则使用 `gcc` 编译 `netbar_billing.c`。
- 启动 `wrapper_server.py`，服务监听 `http://localhost:8000/`。

3. 在浏览器中打开：
```text
http://localhost:8000/
```

## 手动启动
如果你想手动执行，每一步如下：
1. 编译 C 程序：
```batch
gcc -g netbar_billing.c -o netbar_billing.exe
```
2. 启动 Python 封装服务：
```batch
python wrapper_server.py
```
3. 访问 `http://localhost:8000/`

## 默认管理员账号
默认管理员登录密码为：`admin123`
> 管理员登录仅用于查看所有用户、记录和统计信息，并支持强制下线用户。

## 使用说明

### 普通用户
1. 注册账号
2. 登录
3. 充值余额
4. 在“在线上机”页面选择空闲电脑并确认上机
5. 查看当前上机时长和费用
6. 结束上机时结算消费
7. 在历史记录中查看过往消费明细

### 管理员
1. 切换到管理员登录
2. 输入管理员密码 `admin123`
3. 查看用户信息、消费记录、统计数据
4. 如果需要，可选择强制下线某个正在上机的用户

## 数据存储
系统运行时会在当前目录生成以下文件：
- `users.dat`：用户账户信息
- `records.dat`：消费记录
- `pcs.dat`：电脑状态

## 注意事项
- 前端通过 `wrapper_server.py` 调用后端 C 程序，因此必须先生成 `netbar_billing.exe` 或 `netbar_billing`。
- 在 Windows 环境中，`run_wrapper.bat` 是最简单的启动方式。
- 如果出现编译失败，请确认 `gcc` 已安装并在系统 `PATH` 中。

## 可扩展方向
- 增加密码加密存储
- 改进前端界面和表单验证
- 添加用户注册时姓名/手机号等信息
- 支持按分钟计费或多档价格
- 支持更多管理员权限和日志记录
