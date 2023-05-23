for /f %%i in ('python get_latest_model.py') do set model_path=%%i
::set model_path=logs/sac/ProjectD-v0_40/best_model.zip

python train.py --algo sac --env ProjectD-v0 --tensorboard-log logs_tb --save-freq 200000 --eval-freq 20000 --eval-episodes 5 --n-eval-envs 1 -i %model_path% -n 9000000
pause
