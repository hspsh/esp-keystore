import os, json
import subprocess

secrets = subprocess.check_output("doppler secrets download --no-file".split(" "))
# print(secrets)

secrets_json = json.loads(secrets)
# print(secrets_json)

with open("include/secrets.h", 'w', encoding = 'utf-8') as f:
    for k,v in secrets_json.items():
        f.write(f'#define {k} "{v}"\n')
