import subprocess

def run(cmd_args):
    r = subprocess.run(['main_web.exe'] + cmd_args, capture_output=True, encoding='utf-8', errors='replace', timeout=10)
    return r.stdout

# 优先 reset 一下避免前面 toggle 残留
print('=== reset ===')
print(run(['reset']))
print()

for cmd, cmd_args in [
    ('list_lines', []),
    ('close-transfer', ['人民广场']),
    ('line-toggle', ['1号线', 'close']),
    ('line-toggle', ['1号线', 'open']),
    ('network-toggle', ['open']),
    ('impact', ['人民广场', '1号线']),
    ('network', []),
    ('save', []),
]:
    print(f'=== {cmd} {cmd_args} ===')
    print(run([cmd] + cmd_args))


