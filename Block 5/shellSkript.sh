#!/bin/sh
make

session="block4"
# set up tmux
tmux start-server
tmux new-session -d -s $session

tmux selectp -t 1
tmux send-keys "./peer 127.0.0.1 3001 15" C-m

tmux splitw -h
tmux send-keys "sleep 3" C-m
tmux send-keys "./peer 127.0.0.1 3002 60 127.0.0.1 3001" C-m

tmux selectp -t 2
tmux splitw -v

tmux selectp -t 3
tmux send-keys "sleep 7" C-m
tmux send-keys "./peer 127.0.0.1 3003 112 127.0.0.1 3001" C-m

tmux selectp -t 0
tmux splitw -v
tmux send-keys "sleep 9" C-m
tmux send-keys "./peer 127.0.0.1 3004 245 127.0.0.1 3001" C-m

tmux new-window -t $session:1 -n scratch
tmux select-window -t $session:0
tmux attach-session -t $session