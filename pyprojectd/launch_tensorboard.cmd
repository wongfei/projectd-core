for /f %%i in ('python get_latest_tensorboard.py') do set tb_path=%%i
::set tb_path=logs_tb\ProjectD-v0\SAC_38

tensorboard --logdir %tb_path%
pause
