#!/usr/bin/env python3
import http.server
import json
import os
import re
import socketserver
import subprocess
import urllib.parse
from http import HTTPStatus

ROOT = os.path.abspath(os.path.dirname(__file__))
PORT = 8000
EXEC_NAMES = ["netbar_billing.exe", "netbar_billing"]
EXEC_PATH = next((os.path.join(ROOT, name) for name in EXEC_NAMES if os.path.exists(os.path.join(ROOT, name))), None)
ENCODING = "utf-8"


class ApiHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path.startswith('/api/'):
            self.handle_api_get(parsed)
        else:
            super().do_GET()

    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path.startswith('/api/'):
            self.handle_api_post(parsed)
        else:
            self.send_error(HTTPStatus.NOT_FOUND, 'Unknown endpoint')

    def handle_api_get(self, parsed):
        path = parsed.path
        query = urllib.parse.parse_qs(parsed.query)
        try:
            if path == '/api/init':
                data = api_init()
                self.send_json(data)
            elif path == '/api/checkFee':
                payload = {k: v[0] for k, v in query.items()}
                self.send_json(api_check_fee(payload))
            elif path == '/api/history':
                payload = {k: v[0] for k, v in query.items()}
                self.send_json(api_history(payload))
            elif path == '/api/admin/users':
                self.send_json(api_admin_users())
            elif path == '/api/admin/records':
                self.send_json(api_admin_records())
            elif path == '/api/admin/stats':
                self.send_json(api_admin_stats())
            else:
                self.send_error(HTTPStatus.NOT_FOUND, 'Unknown endpoint')
        except Exception as exc:
            self.send_json({'success': False, 'message': str(exc)}, status=500)

    def handle_api_post(self, parsed):
        path = parsed.path
        length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(length).decode('utf-8') if length else ''
        payload = json.loads(body) if body else {}
        try:
            if path == '/api/register':
                self.send_json(api_register(payload))
            elif path == '/api/login':
                self.send_json(api_login(payload))
            elif path == '/api/recharge':
                self.send_json(api_recharge(payload))
            elif path == '/api/start':
                self.send_json(api_start(payload))
            elif path == '/api/stop':
                self.send_json(api_stop(payload))
            elif path == '/api/checkFee':
                self.send_json(api_check_fee(payload))
            elif path == '/api/history':
                self.send_json(api_history(payload))
            elif path == '/api/admin/login':
                self.send_json(api_admin_login(payload))
            elif path == '/api/admin/force_logout':
                self.send_json(api_admin_force_logout(payload))
            else:
                self.send_error(HTTPStatus.NOT_FOUND, 'Unknown endpoint')
        except Exception as exc:
            self.send_json({'success': False, 'message': str(exc)}, status=500)

    def send_json(self, data, status=200):
        body = json.dumps(data, ensure_ascii=False).encode('utf-8')
        self.send_response(status)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)


def exec_path_or_error():
    if EXEC_PATH and os.path.exists(EXEC_PATH):
        return EXEC_PATH
    raise FileNotFoundError('找不到可执行文件 netbar_billing.exe 或 netbar_billing，请先编译 C 程序。')


def run_c_program(commands):
    exe = exec_path_or_error()
    input_text = '\n'.join(str(item) for item in commands) + '\n'
    proc = subprocess.run(
        [exe],
        input=input_text,
        text=True,
        encoding=ENCODING,
        errors='replace',
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=20,
    )
    output = proc.stdout.replace('\r\n', '\n')
    return output


def parse_users_output(output):
    users = []
    collect = False
    for line in output.splitlines():
        if line.strip().startswith('用户名') and '余额' in line and '状态' in line:
            collect = True
            continue
        if collect:
            if line.strip().startswith('---') or not line.strip():
                continue
            parts = re.split(r'\s{2,}', line.strip())
            if len(parts) >= 4:
                username = parts[0].strip()
                try:
                    balance = float(parts[1].strip())
                except ValueError:
                    balance = 0.0
                status = parts[2].strip()
                pc_id = int(parts[3].strip()) if parts[3].strip() != '-1' else None
                users.append({
                    'username': username,
                    'balance': balance,
                    'is_active': status == '上机中',
                    'pc_id': pc_id,
                })
    return users


def parse_records_output(output):
    records = []
    current = {}
    for line in output.splitlines():
        line = line.strip()
        if line.startswith('记录'):
            if current:
                records.append(current)
            current = {}
        elif line.startswith('用户:'):
            current['username'] = line.split('用户:')[1].strip()
        elif line.startswith('电脑:'):
            current['pc_id'] = int(line.split('电脑:')[1].strip())
        elif line.startswith('开始:'):
            current['start_time'] = line.split('开始:')[1].strip()
        elif line.startswith('结束:'):
            current['end_time'] = line.split('结束:')[1].strip()
        elif line.startswith('金额:'):
            try:
                current['amount'] = float(line.split('金额:')[1].replace('元', '').strip())
            except ValueError:
                current['amount'] = 0.0
    if current:
        records.append(current)
    return records


def parse_history_output(output):
    return parse_records_output(output)


def parse_check_fee_output(output):
    result = {}
    for line in output.splitlines():
        if line.strip().startswith('使用电脑:'):
            result['pc_id'] = int(line.split('使用电脑:')[1].strip())
        elif line.strip().startswith('上机时间:'):
            result['start_time'] = line.split('上机时间:')[1].strip()
        elif line.strip().startswith('当前费用:'):
            result['fee'] = float(re.sub('[^0-9.]+', '', line.split('当前费用:')[1]))
        elif line.strip().startswith('当前时长:'):
            result['duration'] = line.split('当前时长:')[1].strip()
    return result


def parse_stats_output(output):
    stats = {}
    for line in output.splitlines():
        if line.strip().startswith('用户总数:'):
            stats['users'] = int(re.sub('[^0-9]', '', line))
        elif line.strip().startswith('消费记录:'):
            stats['records'] = int(re.sub('[^0-9]', '', line))
        elif line.strip().startswith('总收入:'):
            stats['revenue'] = float(re.sub('[^0-9.]+', '', line))
        elif line.strip().startswith('当前上机:'):
            stats['online'] = int(re.sub('[^0-9]', '', line))
        elif line.strip().startswith('空闲电脑:'):
            stats['free'] = int(re.sub('[^0-9]', '', line))
    return stats


def api_init():
    users = api_admin_users()['users']
    records = api_admin_records()['records']
    stats = api_admin_stats()['stats']
    pcs = []
    occupied = {user['pc_id'] for user in users if user.get('is_active') and user.get('pc_id')}
    for i in range(1, 51):
        pcs.append({
            'pc_id': i,
            'is_occupied': i in occupied,
        })
    return {'success': True, 'users': users, 'records': records, 'pcs': pcs, 'stats': stats}


def api_register(payload):
    username = payload.get('username', '').strip()
    password = payload.get('password', '').strip()
    if not username or not password:
        return {'success': False, 'message': '用户名和密码不能为空'}
    output = run_c_program([-1, 1, username, password, 0])
    if '注册成功' in output:
        return {'success': True, 'message': '注册成功'}
    return {'success': False, 'message': '注册失败: ' + extract_error(output)}


def api_login(payload):
    username = payload.get('username', '').strip()
    password = payload.get('password', '').strip()
    if not username or not password:
        return {'success': False, 'message': '用户名和密码不能为空'}
    output = run_c_program([-1, 2, username, password, 6, 0])
    if '登录成功' in output:
        return {'success': True, 'message': '登录成功'}
    return {'success': False, 'message': '登录失败: ' + extract_error(output)}


def api_recharge(payload):
    username = payload.get('username', '').strip()
    password = payload.get('password', '').strip()
    amount = payload.get('amount')
    if not username or not password or amount is None:
        return {'success': False, 'message': '参数不完整'}
    try:
        amount = float(amount)
    except ValueError:
        return {'success': False, 'message': '充值金额无效'}
    output = run_c_program([-1, 2, username, password, 4, amount, 6, 0])
    if '充值成功' in output:
        return {'success': True, 'message': '充值成功'}
    return {'success': False, 'message': '充值失败: ' + extract_error(output)}


def api_start(payload):
    username = payload.get('username', '').strip()
    password = payload.get('password', '').strip()
    pc_id = payload.get('pc_id')
    if not username or not password or pc_id is None:
        return {'success': False, 'message': '参数不完整'}
    try:
        pc_id = int(pc_id)
    except ValueError:
        return {'success': False, 'message': '电脑编号无效'}
    output = run_c_program([-1, 2, username, password, 1, pc_id, 6, 0])
    if '上机成功' in output:
        return {'success': True, 'message': '上机成功'}
    return {'success': False, 'message': '上机失败: ' + extract_error(output)}


def api_stop(payload):
    username = payload.get('username', '').strip()
    password = payload.get('password', '').strip()
    if not username or not password:
        return {'success': False, 'message': '参数不完整'}
    output = run_c_program([-1, 2, username, password, 2, 6, 0])
    if '结账成功' in output:
        return {'success': True, 'message': '下机成功'}
    return {'success': False, 'message': '下机失败: ' + extract_error(output)}


def api_check_fee(payload):
    username = payload.get('username', '').strip()
    password = payload.get('password', '').strip()
    if not username or not password:
        return {'success': False, 'message': '参数不完整'}
    output = run_c_program([-1, 2, username, password, 3, 6, 0])
    if '当前费用' in output:
        data = parse_check_fee_output(output)
        return {'success': True, 'data': data}
    return {'success': False, 'message': '查询失败: ' + extract_error(output)}


def api_history(payload):
    username = payload.get('username', '').strip()
    password = payload.get('password', '').strip()
    if not username or not password:
        return {'success': False, 'message': '参数不完整'}
    output = run_c_program([-1, 2, username, password, 5, 6, 0])
    if '暂无消费记录' in output or '记录' in output:
        records = parse_history_output(output)
        return {'success': True, 'records': records}
    return {'success': False, 'message': '查询失败: ' + extract_error(output)}


def api_admin_login(payload):
    password = payload.get('password', '').strip()
    if password == 'admin123':
        return {'success': True, 'message': '管理员登录成功'}
    return {'success': False, 'message': '管理员密码错误'}


def api_admin_users():
    output = run_c_program([-1, 3, 'admin123', 1, 0, 0])
    users = parse_users_output(output)
    return {'success': True, 'users': users}


def api_admin_records():
    output = run_c_program([-1, 3, 'admin123', 2, 0, 0])
    records = parse_records_output(output)
    return {'success': True, 'records': records}


def api_admin_stats():
    output = run_c_program([-1, 3, 'admin123', 3, 0, 0])
    stats = parse_stats_output(output)
    return {'success': True, 'stats': stats}


def api_admin_force_logout(payload):
    username = payload.get('username', '').strip()
    if not username:
        return {'success': False, 'message': '参数不完整'}
    output = run_c_program([-1, 3, 'admin123', 4, username, 0, 0])
    if '强制下线成功' in output:
        return {'success': True, 'message': '强制下线成功'}
    return {'success': False, 'message': '强制下线失败: ' + extract_error(output)}


def extract_error(output):
    errors = [
        '错误：用户名或密码错误！',
        '错误：余额不足！',
        '错误：无效的电脑编号！',
        '错误：电脑',
        '错误：用户名已存在！',
        '错误：充值金额必须大于0！',
        '错误：您当前没有在上机！',
        '错误：您已经在使用电脑',
    ]
    for err in errors:
        if err in output:
            return err
    return output.strip() or '未返回有效信息'


if __name__ == '__main__':
    os.chdir(ROOT)
    handler = ApiHandler
    with socketserver.TCPServer(('', PORT), handler) as httpd:
        print(f'正在启动封装服务，访问 http://localhost:{PORT}/')
        if EXEC_PATH is None:
            print('警告：未找到 netbar_billing 可执行文件，请先编译 C 程序。')
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print('\n服务已停止。')
