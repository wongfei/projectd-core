python train.py --algo sac --env ProjectD-v0 --tensorboard-log logs_tb --save-freq 200000 --eval-freq 20000 --eval-episodes 5 --n-eval-envs 1
::-optimize --n-trials 1000 --n-jobs 2 --sampler tpe --pruner median
pause
