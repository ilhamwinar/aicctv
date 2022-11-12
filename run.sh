#!bin/bash
docker build -t yudhabhakti/aicctv .
docker run -d --net host --rm --gpus all --name aicctv yudhabhakti/aicctv
