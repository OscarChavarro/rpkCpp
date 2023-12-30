#!/bin/bash

mkdir -p output

./build/rpk -display-lists -iterations 9 -radiance-model-savefile output/oficina2.wrl etc/oficina002/office2.mgf -radiance-method Galerkin -raytracing-method none -batch -batch-quit-at-end -offscreen
