import os, json
import subprocess

#it no work yet
exit()

secrets = subprocess.check_output("doppler secrets download --no-file".split(" "))
# print(secrets)

secrets_json = json.loads(secrets)
# print(secrets_json)

for k,v in secrets_json.items():
    # print(k)
    # print(v)
    env.Append(CPPDEFINES=[
      (k,v)
    ])

# # add macro definition for code to use in projenv (visible to src/ files) 
# projenv.Append(CPPDEFINES=[
#   ("SECRET_VALUE_MACRO", secret)
# ])