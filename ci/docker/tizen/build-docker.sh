#!/bin/bash

IMAGE_NAME=ghcr.io/flutter-tizen/build-engine
IMAGE_TAG=latest

docker build --tag $IMAGE_NAME:$IMAGE_TAG .
