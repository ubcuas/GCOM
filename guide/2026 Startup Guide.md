# Software Preflight Setup

## DRONE

- [ ] Insert LTE Board into miniPCIe module
- [ ] Insert SIM Card into LTE Board
  - Hold it upside down to let gravity help you insert
- [ ] Attach antenna to LTE board
  - > **TODO:** UPDATE HERE ABOUT CORRECT PORTS
- [ ] Attach Jetson to drone
- [ ] Attach camera to mount
- [ ] Connect power to Jetson
- [ ] Connect camera to Jetson

---

## GCOM / PC

- [ ] Connect radio transmitter to thinkpad computer
  - Remember to put arrow to 1 (bottom pin row) on wire
- [ ] Create 2 Terminal tabs for GCOM and Mavproxy
- [ ] Start `mavproxy.py` with port forwarding for Mission Planner and GCOM
  - > **TODO:** Add command here
- [ ] Start Mission Planner and connect to mavproxy
- [ ] Start GCOM with startup script for all 3 services
  - **CHECK:** Ensure all three tmux windows have no errors
- [ ] Start WebRTC signalling server with script + ngrok
  - Potentially update to new IP on server
- [ ] Check GCOM frontend to see if drone made connection

---

## JETSON

- [ ] SSH onto system
- [ ] Run `docker compose up -d` in `hawkeye-os` repo to start ROS container
  - **CHECK:** Check logs to see if container is running properly