for /f %%i in ('python get_latest_exp_id.py') do set exp_id=%%i

python enjoy.py --algo sac --env ProjectD-v0 -f logs/ --exp-id %exp_id% --load-best -n 9000000 --env-kwargs enjoy_flag:1
pause
