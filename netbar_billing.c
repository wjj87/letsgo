

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 常量定义 
#define maxUsers 100               // 最大用户数量
#define maxRecords 1000            // 最大消费记录数量
#define #sym:useNameLen 32             // 用户名长度
#define PASSWORD_LEN 32             // 密码长度
#define MAX_PC 50                   // 网吧电脑数量
#define per_hour 5.0           // 每小时价格是5元


// 用户
typedef struct {
    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];
    double balance;  // 余额
    int is_active;   // 是否正在上网
    int pc_id;       // 电脑编号
    time_t login_time;  // 上机时间
} User;

// 消费记录
typedef struct {
    char username[USERNAME_LEN];
    int pc_id;
    time_t start_time;
    time_t end_time;
    double amount;   // 消费金额
    int is_paid;     // 是否已结账
} Record;

// 电脑状态
typedef struct {
    int pc_id;
    int is_occupied;  // 是否被占用
    char username[USERNAME_LEN];  // 使用者用户名
    time_t start_time;  // 开始使用时间
} PCStatus;

//全局变量 
static User users[maxUsers];//所有的用户信息
static int user_count = 0;//现在的用户数量
static Record records[maxRecords];//所有的消费记录
static int record_count = 0;
static PCStatus pcs[MAX_PC];


//主体操作部分

//数据的保存
// 保存用户数据到文件
void save_users() {
    FILE *fp = fopen("users.dat", "wb");
    if (fp == NULL) {
        printf("错误：无法保存用户数据！\n");
        return;
    }
    fwrite(&user_count, sizeof(int), 1, fp);
    fwrite(users, sizeof(User), user_count, fp);
    fclose(fp);
    printf("用户数据已保存。\n");
}

// 从文件加载用户数据
void load_users() {
    FILE *fp = fopen("users.dat", "rb");
    if (fp == NULL) {
        printf("首次运行，无历史用户数据。\n");
        return;
    }
    fread(&user_count, sizeof(int), 1, fp);
    fread(users, sizeof(User), user_count, fp);
    fclose(fp);
    printf("已加载 %d 个用户数据。\n", user_count);
}

// 保存消费记录到文件
void save_records() {
    FILE *fp = fopen("records.dat", "wb");
    if (fp == NULL) {
        printf("错误：无法保存消费记录！\n");
        return;
    }
    fwrite(&record_count, sizeof(int), 1, fp);
    fwrite(records, sizeof(Record), record_count, fp);
    fclose(fp);
}

// 从文件加载消费记录
void load_records() {
    FILE *fp = fopen("records.dat", "rb");
    if (fp == NULL) {
        return;
    }
    fread(&record_count, sizeof(int), 1, fp);
    fread(records, sizeof(Record), record_count, fp);
    fclose(fp);
}

// 保存电脑状态
void save_pcs() {
    FILE *fp = fopen("pcs.dat", "wb");
    if (fp == NULL) return;
    fwrite(pcs, sizeof(PCStatus), MAX_PC, fp);
    fclose(fp);
}

// 加载电脑状态
void load_pcs() {
    FILE *fp = fopen("pcs.dat", "rb");
    if (fp == NULL) {
        // 初始化电脑状态
        for (int i = 0; i < MAX_PC; i++) {
            pcs[i].pc_id = i + 1;
            pcs[i].is_occupied = 0;
            strcpy(pcs[i].username, "");
            pcs[i].start_time = 0;
        }
        return;
    }
    fread(pcs, sizeof(PCStatus), MAX_PC, fp);
    fclose(fp);
}



// 注册新用户
int register_user() {
    if (user_count >= maxUsers) {
        printf("错误：用户数量已达上限！\n");
        return 0;
    }

    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];

    printf("\n========== 用户注册 ==========\n");
    printf("请输入用户名（最多31个字符）: ");
    scanf("%s", username);

    // 检查用户名是否已存在
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            printf("错误：用户名已存在！\n");
            return 0;
        }
    }

    printf("请输入密码（最多31个字符）: ");
    scanf("%s", password);

    // 创建新用户
    strcpy(users[user_count].username, username);
    strcpy(users[user_count].password, password);
    users[user_count].balance = 0.0;
    users[user_count].is_active = 0;
    users[user_count].pc_id = -1;
    users[user_count].login_time = 0;

    user_count++;
    save_users();

    printf("注册成功！\n");
    return 1;
}

// 用户登录
int login_user(char *logged_username) {
    char username[USERNAME_LEN];
    char password[PASSWORD_LEN];

    printf("\n========== 用户登录 ==========\n");
    printf("请输入用户名: ");
    scanf("%s", username);
    printf("请输入密码: ");
    scanf("%s", password);

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 &&
            strcmp(users[i].password, password) == 0) {
            strcpy(logged_username, username);
            printf("登录成功！欢迎 %s\n", username);
            return i;
        }
    }

    printf("错误：用户名或密码错误！\n");
    return -1;
}

// 充值
void recharge(int user_idx) {
    double amount;
    printf("\n========== 账户充值 ==========\n");
    printf("当前余额: %.2f 元\n", users[user_idx].balance);
    printf("请输入充值金额: ");
    scanf("%lf", &amount);

    if (amount <= 0) {
        printf("错误：充值金额必须大于0！\n");
        return;
    }

    users[user_idx].balance += amount;
    save_users();
    printf("充值成功！当前余额: %.2f 元\n", users[user_idx].balance);
}



//上机计费
// 计算上网费用
double calculate_fee(time_t start_time) {
    time_t now = time(NULL);
    double hours = difftime(now, start_time) / 3600.0;
    // 不足1小时按1小时计费，超过部分按分钟计算
    return hours * per_hour;
}

// 显示空闲电脑
void show_free_pcs() {
    printf("\n========== 空闲电脑列表 ==========\n");
    int count = 0;
    for (int i = 0; i < MAX_PC; i++) {
        if (!pcs[i].is_occupied) {
            printf("电脑 %d ", pcs[i].pc_id);
            count++;
        }
    }
    if (count == 0) {
        printf("暂无可用电脑！\n");
    } else {
        printf("\n共有 %d 台空闲电脑\n", count);
    }
}

// 上机
int login_pc(int user_idx) {
    if (users[user_idx].is_active) {
        printf("您已经在使用电脑 %d，无法重复上机！\n", users[user_idx].pc_id);
        return 0;
    }

    show_free_pcs();

    int pc_id;
    printf("\n请选择电脑编号 (1-%d): ", MAX_PC);
    scanf("%d", &pc_id);

    if (pc_id < 1 || pc_id > MAX_PC) {
        printf("错误：无效的电脑编号！\n");
        return 0;
    }

    if (pcs[pc_id - 1].is_occupied) {
        printf("错误：电脑 %d 已被占用！\n", pc_id);
        return 0;
    }

    // 计算当前费用
    double current_fee = calculate_fee(time(NULL));

    if (users[user_idx].balance < current_fee + per_hour) {
        printf("余额不足！当前余额: %.2f 元，至少需要: %.2f 元\n",
               users[user_idx].balance, current_fee + per_hour);
        return 0;
    }

    // 上机
    pcs[pc_id - 1].is_occupied = 1;
    strcpy(pcs[pc_id - 1].username, users[user_idx].username);
    pcs[pc_id - 1].start_time = time(NULL);

    users[user_idx].is_active = 1;
    users[user_idx].pc_id = pc_id;
    users[user_idx].login_time = pcs[pc_id - 1].start_time;

    save_pcs();
    save_users();

    printf("\n上机成功！\n");
    printf("电脑编号: %d\n", pc_id);
    printf("上机时间: %s", ctime(&users[user_idx].login_time));
    printf("费率: %.2f 元/小时\n", per_hour);

    return 1;
}

// 下机结账
int logout_pc(int user_idx) {
    if (!users[user_idx].is_active) {
        printf("您当前没有在上机！\n");
        return 0;
    }

    time_t now = time(NULL);
    double hours = difftime(now, users[user_idx].login_time) / 3600.0;
    double fee = hours * per_hour;

    printf("\n========== 下机结账 ==========\n");
    printf("使用电脑: %d\n", users[user_idx].pc_id);
    printf("上机时间: %s", ctime(&users[user_idx].login_time));
    printf("下机时间: %s", ctime(&now));
    printf("使用时长: %.2f 小时\n", hours);
    printf("消费金额: %.2f 元\n", fee);
    printf("当前余额: %.2f 元\n", users[user_idx].balance);

    if (users[user_idx].balance < fee) {
        printf("错误：余额不足！请先充值！\n");
        return 0;
    }

    // 扣除费用
    users[user_idx].balance -= fee;

    // 创建消费记录
    if (record_count < maxRecords) {
        strcpy(records[record_count].username, users[user_idx].username);
        records[record_count].pc_id = users[user_idx].pc_id;
        records[record_count].start_time = users[user_idx].login_time;
        records[record_count].end_time = now;
        records[record_count].amount = fee;
        records[record_count].is_paid = 1;
        record_count++;
    }

    // 更新电脑状态
    int pc_idx = users[user_idx].pc_id - 1;
    pcs[pc_idx].is_occupied = 0;
    strcpy(pcs[pc_idx].username, "");
    pcs[pc_idx].start_time = 0;

    // 更新用户状态
    users[user_idx].is_active = 0;
    users[user_idx].pc_id = -1;
    users[user_idx].login_time = 0;

    save_users();
    save_pcs();
    save_records();

    printf("结账成功！剩余余额: %.2f 元\n", users[user_idx].balance);

    return 1;
}

// 查看当前费用
void check_current_fee(int user_idx) {
    if (!users[user_idx].is_active) {
        printf("您当前没有在上机！\n");
        return;
    }

    time_t now = time(NULL);
    double hours = difftime(now, users[user_idx].login_time) / 3600.0;
    double fee = hours * per_hour;

    printf("\n========== 当前费用查询 ==========\n");
    printf("使用电脑: %d\n", users[user_idx].pc_id);
    printf("上机时间: %s", ctime(&users[user_idx].login_time));
    printf("当前时长: %.2f 小时\n", hours);
    printf("当前费用: %.2f 元\n", fee);
    printf("账户余额: %.2f 元\n", users[user_idx].balance);
}

// 查看历史记录
void show_history(int user_idx) {
    printf("\n========== 消费历史记录 ==========\n");
    int count = 0;
    for (int i = 0; i < record_count; i++) {
        if (strcmp(records[i].username, users[user_idx].username) == 0) {
            printf("\n记录 %d:\n", ++count);
            printf("  电脑编号: %d\n", records[i].pc_id);
            printf("  开始时间: %s", ctime(&records[i].start_time));
            printf("  结束时间: %s", ctime(&records[i].end_time));
            printf("  消费金额: %.2f 元\n", records[i].amount);
            printf("  状态: %s\n", records[i].is_paid ? "已结账" : "未结账");
        }
    }
    if (count == 0) {
        printf("暂无消费记录！\n");
    } else {
        printf("\n共 %d 条记录\n", count);
    }
}




//用户管理
// 管理员登录验证
int admin_login() {
    char password[PASSWORD_LEN];
    printf("\n========== 管理员登录 ==========\n");
    printf("请输入管理员密码: ");
    scanf("%s", password);

    // 默认管理员密码: admin123
    if (strcmp(password, "admin123") == 0) {
        printf("管理员登录成功！\n");
        return 1;
    }
    printf("密码错误！\n");
    return 0;
}

// 查看所有用户
void show_all_users() {
    printf("\n========== 所有用户列表 ==========\n");
    printf("%-20s %-15s %-15s %-10s\n", "用户名", "余额", "状态", "电脑");
    printf("--------------------------------------------------------------\n");
    for (int i = 0; i < user_count; i++) {
        printf("%-20s %-15.2f %-15s %-10d\n",
               users[i].username,
               users[i].balance,
               users[i].is_active ? "上机中" : "空闲",
               users[i].is_active ? users[i].pc_id : -1);
    }
}

// 查看所有消费记录
void show_all_records() {
    printf("\n========== 所有消费记录 ==========\n");
    if (record_count == 0) {
        printf("暂无消费记录！\n");
        return;
    }
    for (int i = 0; i < record_count; i++) {
        printf("\n记录 %d:\n", i + 1);
        printf("  用户: %s\n", records[i].username);
        printf("  电脑: %d\n", records[i].pc_id);
        printf("  开始: %s", ctime(&records[i].start_time));
        printf("  结束: %s", ctime(&records[i].end_time));
        printf("  金额: %.2f 元\n", records[i].amount);
    }
}

// 统计数据
void show_statistics() {
    printf("\n========== 系统统计 ==========\n");
    printf("用户总数: %d\n", user_count);
    printf("消费记录: %d 条\n", record_count);

    double total_revenue = 0;
    for (int i = 0; i < record_count; i++) {
        total_revenue += records[i].amount;
    }
    printf("总收入: %.2f 元\n", total_revenue);

    int active_users = 0;
    for (int i = 0; i < user_count; i++) {
        if (users[i].is_active) active_users++;
    }
    printf("当前上机: %d 人\n", active_users);
    printf("空闲电脑: %d 台\n", MAX_PC - active_users);
}





//主界面
// 打印主菜单
void print_main_menu() {
    printf("\n");
    printf("===========================================\n");
    printf("       网吧上网计费系统 - 主菜单\n");
    printf("===========================================\n");
    printf("  1. 用户注册\n");
    printf("  2. 用户登录\n");
    printf("  3. 管理员登录\n");
    printf("  0. 退出系统\n");
    printf("===========================================\n");
    printf("请选择: ");
}

// 打印用户菜单
void print_user_menu() {
    printf("\n");
    printf(" !!!!!!!用户菜单!!!!!!!!\n");
    printf("  1. 上机\n");
    printf("  2. 下机结账\n");
    printf("  3. 查询当前费用\n");
    printf("  4. 账户充值\n");
    printf("  5. 消费记录查询\n");
    printf("  6. 退出登录\n");
    printf("===========================================\n");
    printf("请选择: ");
}

// 打印管理员菜单
void print_admin_menu() {
    printf("\n");
    printf("!!!!!!!!!管理员菜单!!!!!!!!!\n");
    printf("  1. 查看所有用户\n");
    printf("  2. 查看所有消费记录\n");
    printf("  3. 查看系统统计\n");
    printf("  4. 强制下线用户\n");
    printf("  0. 退出管理\n");
    printf("===========================================\n");
    printf("请选择: ");
}

void print_three_menu(){
    printf("\n");
    printf("打印主菜单输入-1\n");
    printf("打印管理员菜单输入-2\n");
    printf("打印用户菜单输入机器编号\n");
    printf("\n");
}

// 强制下线用户（管理员功能）
void force_logout() {
show_all_users();
    printf("\n请输入要强制下线的用户名: ");
    char username[USERNAME_LEN];
    scanf("%s", username);//输入用户名

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            if (!users[i].is_active) {
                printf("用户 %s 当前不在线！\n", username);
                return;
            }

            time_t now = time(NULL);
            double hours = difftime(now, users[i].login_time) / 3600.0;
            double fee = hours * per_hour;

            printf("警告：强制下线用户 %s\n", username);
            printf("消费金额: %.2f 元将从余额中扣除！\n", fee);

            // 创建记录
            if (record_count < maxRecords) {
                strcpy(records[record_count].username, users[i].username);
                records[record_count].pc_id = users[i].pc_id;
                records[record_count].start_time = users[i].login_time;
                records[record_count].end_time = now;
                records[record_count].amount = fee;
                records[record_count].is_paid = 1;
                record_count++;
            }

            // 更新电脑状态
            int pc_idx = users[i].pc_id - 1;
            pcs[pc_idx].is_occupied = 0;
            strcpy(pcs[pc_idx].username, "");

            // 更新用户状态
            users[i].is_active = 0;
            users[i].pc_id = -1;
            users[i].login_time = 0;

            save_users();
            save_pcs();
            save_records();

            printf("强制下线成功！\n");
            return;
        }
    }
    printf("未找到用户: %s\n", username);
}

// 主函数 
int main() {
    printf("===========================================\n");
    printf("       欢迎使用网吧上网计费系统\n");
    printf("===========================================\n");

    // 加载数据
    load_users();//加载用户数据
    load_records();//加载消费记录
    load_pcs();//加载电脑状态

    print_three_menu();
    int choice;
    int logged_in;  // 登录用户索引
    scanf("%d", &logged_in);
    char current_user[USERNAME_LEN] = "";// 当前登录的用户名
    while (1) {
        if (logged_in == -1) {
            // 主菜单
            print_main_menu();
            scanf("%d", &choice);
            switch (choice) {
                case 1:
                    register_user();//注册新用户
                    break;
                case 2:
                    logged_in = login_user(current_user);//实现登录
                    break;
                case 3:
                    if (admin_login()) {//管理员登录！！！衔接
                        logged_in = -2;  // 管理员模式
                    }
                    break;
                case 0:
                    printf("感谢使用，再见！\n");
                    return 0;
                default:
                    printf("无效选择，请重试！\n");
            }
        } else if (logged_in == -2) {
            // 管理员菜单
            print_admin_menu();
            scanf("%d", &choice);

            switch (choice) {
                case 1:
                    show_all_users();//查看所有用户
                    break;
                case 2:
                    show_all_records();//查看所有消费记录
                    break;
                case 3:
                    show_statistics();//查看系统统计
                    break;
                case 4:
                    force_logout();//强制下线用户
                    break;
                case 0:
                    logged_in = -1;
                    printf("已退出管理模式！\n");
                    break;
                default:
                    printf("无效选择，请重试！\n");
            }
        } else {
            // 用户菜单
            printf("\n当前用户: %s  余额: %.2f 元\n",
                   users[logged_in].username,
                   users[logged_in].balance);
            if (users[logged_in].is_active) {
                printf("状态: 上机中 (电脑 %d)\n", users[logged_in].pc_id);
            } else {
                printf("状态: 空闲\n");
            }

            print_user_menu();
            scanf("%d", &choice);

            switch (choice) {
                case 1:
                    login_pc(logged_in);//上机
                    break;
                case 2:
                    logout_pc(logged_in);//下机
                    break;
                case 3:
                    check_current_fee(logged_in);//查询当前费用
                    break;
                case 4:
                    recharge(logged_in);//充值
                    break;
                case 5:
                    show_history(logged_in);//查看历史记录
                    break;
                case 6:
                    logged_in = -1;
                    strcpy(current_user, "");
                    printf("已退出登录！\n");
                    break;
                default:
                    printf("无效选择，请重试！\n");
            }
        }
    }
    return 0;
}