@echo off
python3 main.py -m socket -p pub -i 127.0.0.1 -port 1883 -t 100 -inf RSU_Messages.json -freq 10 -size 1024
