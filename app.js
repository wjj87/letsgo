const API_BASE = '';
const RATE_PER_HOUR = 5;
const MAX_PC = 50;

let currentUser = null;
let isAdmin = false;
let users = [];
let records = [];
let pcs = [];
let selectedPc = null;
let onlineTimer = null;

async function apiRequest(path, options = {}) {
    const init = {
        method: options.method || 'GET',
        headers: {
            'Content-Type': 'application/json',
        },
    };
    if (options.body) {
        init.body = JSON.stringify(options.body);
    }
    const response = await fetch(API_BASE + path, init);
    const data = await response.json();
    if (!response.ok) {
        throw new Error(data.message || '请求失败');
    }
    return data;
}

async function init() {
    await loadData();
    renderPCGrid();
    updateStatus();
}

async function loadData() {
    const data = await apiRequest('/api/init');
    users = data.users || [];
    records = data.records || [];
    pcs = data.pcs || [];
    syncCurrentUser();
}

function syncCurrentUser() {
    if (!currentUser) return;
    const user = users.find(u => u.username === currentUser.username);
    if (user) {
        currentUser = { ...currentUser, ...user };
    }
}

function switchTab(tab, event) {
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    const target = event ? event.target : window.event && window.event.target;
    if (target) {
        target.classList.add('active');
    }

    document.getElementById('loginForm').style.display = 'none';
    document.getElementById('registerForm').style.display = 'none';
    document.getElementById('adminForm').style.display = 'none';

    if (tab === 'login') {
        document.getElementById('loginForm').style.display = 'block';
    } else if (tab === 'register') {
        document.getElementById('registerForm').style.display = 'block';
    } else if (tab === 'admin') {
        document.getElementById('adminForm').style.display = 'block';
    }
}

function showToast(message) {
    document.getElementById('toastMessage').textContent = message;
    document.getElementById('toast').classList.add('active');
}

function closeToast() {
    document.getElementById('toast').classList.remove('active');
}

async function showSection(section) {
    document.querySelectorAll('.section').forEach(s => s.classList.remove('active'));
    document.getElementById(section + 'Section').classList.add('active');

    if (section === 'online') {
        renderPCGrid();
        await updateOnlineInfo();
    } else if (section === 'recharge') {
        document.getElementById('rechargeBalance').textContent = currentUser ? currentUser.balance.toFixed(2) : '0.00';
    } else if (section === 'history') {
        await renderHistory();
    } else if (section === 'adminUsers') {
        await renderUsersTable();
    } else if (section === 'adminRecords') {
        await renderRecordsTable();
    } else if (section === 'adminStats') {
        await updateStats();
    }

    if (onlineTimer) {
        clearInterval(onlineTimer);
        onlineTimer = null;
    }
    if (section === 'online' && currentUser && currentUser.is_active) {
        onlineTimer = setInterval(updateOnlineInfo, 5000);
    }
}

async function userRegister() {
    const username = document.getElementById('regUsername').value.trim();
    const password = document.getElementById('regPassword').value;
    const password2 = document.getElementById('regPassword2').value;

    if (!username || !password) {
        showToast('用户名和密码不能为空！');
        return;
    }
    if (password !== password2) {
        showToast('两次密码输入不一致！');
        return;
    }

    try {
        const result = await apiRequest('/api/register', {
            method: 'POST',
            body: { username, password },
        });
        if (result.success) {
            showToast('注册成功！请登录。');
            document.getElementById('regUsername').value = '';
            document.getElementById('regPassword').value = '';
            document.getElementById('regPassword2').value = '';
            switchTab('login');
            document.querySelectorAll('.tab')[0].click();
        } else {
            showToast(result.message || '注册失败');
        }
    } catch (error) {
        showToast(error.message);
    }
}

async function userLogin() {
    const username = document.getElementById('loginUsername').value.trim();
    const password = document.getElementById('loginPassword').value;

    if (!username || !password) {
        showToast('用户名和密码不能为空！');
        return;
    }

    try {
        const result = await apiRequest('/api/login', {
            method: 'POST',
            body: { username, password },
        });
        if (result.success) {
            currentUser = { username, password };
            isAdmin = false;
            await loadData();
            updateStatus();
            showSection('user');
            showToast('登录成功！');
        } else {
            showToast(result.message || '登录失败');
        }
    } catch (error) {
        showToast(error.message);
    }
}

async function adminLogin() {
    const password = document.getElementById('adminPassword').value;
    try {
        const result = await apiRequest('/api/admin/login', {
            method: 'POST',
            body: { password },
        });
        if (result.success) {
            isAdmin = true;
            currentUser = null;
            await loadData();
            showSection('admin');
            showToast('管理员登录成功！');
        } else {
            showToast(result.message || '管理员登录失败');
        }
    } catch (error) {
        showToast(error.message);
    }
}

function logout() {
    if (onlineTimer) {
        clearInterval(onlineTimer);
        onlineTimer = null;
    }
    currentUser = null;
    isAdmin = false;
    showSection('auth');
}

function adminLogout() {
    isAdmin = false;
    currentUser = null;
    showSection('auth');
}

function updateStatus() {
    if (currentUser) {
        document.getElementById('currentUser').textContent = '用户: ' + currentUser.username;
        document.getElementById('currentBalance').textContent = '余额: ' + (currentUser.balance || 0).toFixed(2) + ' 元';
    }
}

function renderPCGrid() {
    const grid = document.getElementById('pcGrid');
    grid.innerHTML = '';

    pcs.forEach((pc) => {
        const div = document.createElement('div');
        div.className = 'pc ' + (pc.is_occupied ? 'occupied' : '');
        if (selectedPc === pc.pc_id && !pc.is_occupied) {
            div.classList.add('selected');
        }
        div.textContent = pc.pc_id;
        div.onclick = () => selectPc(pc.pc_id);
        grid.appendChild(div);
    });
}

function selectPc(pcId) {
    const pc = pcs.find(p => p.pc_id === pcId);
    if (pc.is_occupied) {
        showToast('该电脑已被使用！');
        return;
    }
    if (!currentUser) {
        showToast('请先登录');
        return;
    }
    if (currentUser.is_active) {
        showToast('您已经在使用电脑 ' + currentUser.pc_id + '！');
        return;
    }

    selectedPc = pcId;
    renderPCGrid();

    document.getElementById('selectPcInfo').innerHTML = `
                <p style="color:#f39c12;">已选择电脑 ${pcId} 号</p>
                <button class="btn" onclick="doOnline()">确认上机</button>
            `;
}

async function doOnline() {
    if (!selectedPc) {
        showToast('请先选择电脑！');
        return;
    }
    if (!currentUser) {
        showToast('请先登录');
        return;
    }

    try {
        const result = await apiRequest('/api/start', {
            method: 'POST',
            body: {
                username: currentUser.username,
                password: currentUser.password,
                pc_id: selectedPc,
            },
        });
        if (result.success) {
            await loadData();
            updateStatus();
            renderPCGrid();
            await updateOnlineInfo();
            showToast('上机成功！');
        } else {
            showToast(result.message || '上机失败');
        }
    } catch (error) {
        showToast(error.message);
    }
}

async function updateOnlineInfo() {
    if (!currentUser || !currentUser.is_active) {
        document.getElementById('onlineInfo').style.display = 'none';
        return;
    }
    try {
        const result = await apiRequest('/api/checkFee', {
            method: 'POST',
            body: {
                username: currentUser.username,
                password: currentUser.password,
            },
        });
        if (result.success && result.data) {
            document.getElementById('onlineInfo').style.display = 'block';
            document.getElementById('onlinePcId').textContent = result.data.pc_id || currentUser.pc_id || '-';
            document.getElementById('onlineStartTime').textContent = result.data.start_time || '-';
            document.getElementById('onlineDuration').textContent = result.data.duration || '0分0秒';
            document.getElementById('onlineFee').textContent = (result.data.fee || 0).toFixed(2) + ' 元';
        } else {
            document.getElementById('onlineInfo').style.display = 'none';
        }
    } catch (error) {
        document.getElementById('onlineInfo').style.display = 'none';
    }
}

async function offline() {
    if (!currentUser || !currentUser.is_active) {
        showToast('您当前没有在上机！');
        return;
    }
    try {
        const result = await apiRequest('/api/stop', {
            method: 'POST',
            body: {
                username: currentUser.username,
                password: currentUser.password,
            },
        });
        if (result.success) {
            await loadData();
            updateStatus();
            renderPCGrid();
            showToast('下机成功！');
        } else {
            showToast(result.message || '下机失败');
        }
    } catch (error) {
        showToast(error.message);
    }
}

async function checkFee() {
    if (!currentUser || !currentUser.is_active) {
        showToast('您当前没有在上机！');
        return;
    }
    try {
        const result = await apiRequest('/api/checkFee', {
            method: 'POST',
            body: {
                username: currentUser.username,
                password: currentUser.password,
            },
        });
        if (result.success && result.data) {
            showToast(`当前费用: ${result.data.fee.toFixed(2)} 元\n已用时长: ${result.data.duration}`);
        } else {
            showToast(result.message || '查询失败');
        }
    } catch (error) {
        showToast(error.message);
    }
}

function selectRecharge(amount) {
    document.getElementById('customAmount').value = amount;
}

async function doRecharge() {
    if (!currentUser) {
        showToast('请先登录');
        return;
    }
    const amount = parseFloat(document.getElementById('customAmount').value);
    if (!amount || amount <= 0) {
        showToast('请输入有效的充值金额！');
        return;
    }
    try {
        const result = await apiRequest('/api/recharge', {
            method: 'POST',
            body: {
                username: currentUser.username,
                password: currentUser.password,
                amount,
            },
        });
        if (result.success) {
            await loadData();
            updateStatus();
            document.getElementById('rechargeBalance').textContent = currentUser.balance.toFixed(2);
            document.getElementById('customAmount').value = '';
            showToast(`充值成功！当前余额: ${currentUser.balance.toFixed(2)} 元`);
        } else {
            showToast(result.message || '充值失败');
        }
    } catch (error) {
        showToast(error.message);
    }
}

async function renderHistory() {
    const list = document.getElementById('historyList');
    if (!currentUser) {
        list.innerHTML = '<p style="text-align:center;color:#888;">请先登录</p>';
        return;
    }
    try {
        const result = await apiRequest('/api/history', {
            method: 'POST',
            body: {
                username: currentUser.username,
                password: currentUser.password,
            },
        });
        if (!result.success) {
            list.innerHTML = '<p style="text-align:center;color:#888;">查询失败</p>';
            return;
        }
        const userRecords = (result.records || []).filter(r => r.username === currentUser.username);
        if (userRecords.length === 0) {
            list.innerHTML = '<p style="text-align:center;color:#888;">暂无消费记录</p>';
            return;
        }
        list.innerHTML = userRecords.map((r, i) => `
            <div class="history-item">
                <div class="info-row">
                    <span>记录 ${userRecords.length - i}</span>
                    <span>电脑 ${r.pc_id}号</span>
                </div>
                <div class="info-row">
                    <span>开始: ${r.start_time}</span>
                    <span>结束: ${r.end_time}</span>
                </div>
                <div class="info-row">
                    <span>消费金额</span>
                    <span style="color:#2ecc71;">${r.amount.toFixed(2)} 元</span>
                </div>
            </div>
        `).join('');
    } catch (error) {
        list.innerHTML = '<p style="text-align:center;color:#888;">请求失败</p>';
    }
}

async function renderUsersTable() {
    const table = document.getElementById('usersTable');
    if (users.length === 0) {
        table.innerHTML = '<p style="text-align:center;color:#888;">暂无用户</p>';
        return;
    }
    table.innerHTML = `
        <table style="width:100%;border-collapse:collapse;">
            <tr style="background:rgba(0,212,255,0.2);">
                <th style="padding:10px;text-align:left;">用户名</th>
                <th style="padding:10px;">余额</th>
                <th style="padding:10px;">状态</th>
                <th style="padding:10px;">电脑</th>
            </tr>
            ${users.map(u => `
                <tr style="border-bottom:1px solid rgba(255,255,255,0.1);">
                    <td style="padding:10px;">${u.username}</td>
                    <td style="padding:10px;text-align:center;">${u.balance.toFixed(2)}</td>
                    <td style="padding:10px;text-align:center;color:${u.is_active ? '#e74c3c' : '#2ecc71'};">${u.is_active ? '上机中' : '空闲'}</td>
                    <td style="padding:10px;text-align:center;">${u.is_active ? u.pc_id : '-'}</td>
                </tr>
            `).join('')}
        </table>
    `;
}

async function renderRecordsTable() {
    const table = document.getElementById('recordsTable');
    if (records.length === 0) {
        table.innerHTML = '<p style="text-align:center;color:#888;">暂无消费记录</p>';
        return;
    }
    table.innerHTML = `
        <table style="width:100%;border-collapse:collapse;">
            <tr style="background:rgba(0,212,255,0.2);">
                <th style="padding:10px;">用户</th>
                <th style="padding:10px;">电脑</th>
                <th style="padding:10px;">金额</th>
                <th style="padding:10px;">结束时间</th>
            </tr>
            ${records.map(r => `
                <tr style="border-bottom:1px solid rgba(255,255,255,0.1);">
                    <td style="padding:10px;">${r.username}</td>
                    <td style="padding:10px;text-align:center;">${r.pc_id}</td>
                    <td style="padding:10px;text-align:center;color:#2ecc71;">${r.amount.toFixed(2)}</td>
                    <td style="padding:10px;">${r.end_time}</td>
                </tr>
            `).join('')}
        </table>
    `;
}

async function updateStats() {
    document.getElementById('statUsers').textContent = users.length;
    document.getElementById('statRecords').textContent = records.length;
    const totalRevenue = records.reduce((sum, r) => sum + (r.amount || 0), 0);
    document.getElementById('statRevenue').textContent = totalRevenue.toFixed(2) + ' 元';
    const onlineUsers = users.filter(u => u.is_active).length;
    document.getElementById('statOnline').textContent = onlineUsers + ' 人';
    document.getElementById('statFree').textContent = (MAX_PC - onlineUsers) + ' 台';
}

function formatTime(date) {
    const y = date.getFullYear();
    const m = String(date.getMonth() + 1).padStart(2, '0');
    const d = String(date.getDate()).padStart(2, '0');
    const h = String(date.getHours()).padStart(2, '0');
    const min = String(date.getMinutes()).padStart(2, '0');
    const s = String(date.getSeconds()).padStart(2, '0');
    return `${y}-${m}-${d} ${h}:${min}:${s}`;
}

init().catch(error => showToast(error.message));
