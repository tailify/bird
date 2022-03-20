#!/usr/bin/env bash

set -x -e

tag=$1
git tag -f "$tag" 
git push -f origin "$tag"

image_id="ghcr.io/tailify/bird:$tag"
docker build -t "$image_id" --build-arg COMMIT=$tag .
docker push "$image_id"
