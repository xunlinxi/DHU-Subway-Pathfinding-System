import subprocess, time, socket, urllib.request, json

# 后台启动 server
p = subprocess.Popen(['python', 'web/server.py'],
                     stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                     encoding='utf-8', errors='replace')
for i in range(30):
    time.sleep(0.5)
    s = socket.socket()
    try:
        s.connect(('127.0.0.1', 3724)); s.close(); print('port open after', i*0.5, 's'); break
    except: s.close()
else:
    print('PORT NEVER OPENED')
    p.terminate(); out, _ = p.communicate(timeout=2); print(out); raise SystemExit(1)

BASE = 'http://127.0.0.1:3724'

def get(path):
    r = urllib.request.urlopen(BASE + path, timeout=5)
    return r.status, r.read().decode('utf-8')

def post(path, body):
    data = json.dumps(body).encode('utf-8') if body is not None else b''
    req = urllib.request.Request(BASE + path, method='POST', data=data,
                                 headers={'Content-Type':'application/json'})
    try:
        r = urllib.request.urlopen(req, timeout=5)
        return r.status, r.read().decode('utf-8')
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode('utf-8')

print('--- GET /api/subway/lines ---')
s, b = get('/api/subway/lines'); print(s, b[:200], '...')
print('--- POST /api/subway/station/close-transfer {name:人民广场} ---')
s, b = post('/api/subway/station/close-transfer', {'name':'人民广场'}); print(s, b)
print('--- POST /api/subway/line/toggle {line:1号线,action:close} ---')
s, b = post('/api/subway/line/toggle', {'line':'1号线', 'action':'close'}); print(s, b)
print('--- POST /api/subway/line/toggle {line:1号线,action:open} ---')
s, b = post('/api/subway/line/toggle', {'line':'1号线', 'action':'open'}); print(s, b)
print('--- POST /api/subway/network/toggle {action:open} ---')
s, b = post('/api/subway/network/toggle', {'action':'open'}); print(s, b)
print('--- POST /api/subway/analyze/impact {name:人民广场,line:1号线} ---')
s, b = post('/api/subway/analyze/impact', {'name':'人民广场', 'line':'1号线'}); print(s, b[:500])
print('--- GET /api/subway/analyze/network ---')
s, b = get('/api/subway/analyze/network'); print(s, b[:500])
print('--- POST /api/subway/save ---')
s, b = post('/api/subway/save', {}); print(s, b)
print('--- POST /api/subway/reset ---')
s, b = post('/api/subway/reset', {}); print(s, b)

p.terminate(); time.sleep(0.5)
if p.poll() is None: p.kill()
