#!/bin/bash

IMAGE_NAME=ghcr.io/flutter-tizen/build-engine
IMAGE_TAG=latest

docker pull $IMAGE_NAME:$IMAGE_TAG
docker build --tag $IMAGE_NAME:$IMAGE_TAG .
