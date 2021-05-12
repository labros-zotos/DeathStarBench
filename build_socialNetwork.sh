#!/bin/bash
cd socialNetwork
./reset_docker.sh
cd ..
docker image rm social-microservices:latest -f
docker image rm labroszotos/social-microservices:latest -f
docker image build socialNetwork --tag social-microservices:latest
docker tag social-microservices:latest labroszotos/social-microservices:latest
docker push labroszotos/social-microservices:latest
cd socialNetwork
docker-compose up -d 
