#!/bin/bash

mkdir -p output

./build/rpk -display-lists -iterations 9 etc/oficina003/office.mgf -radiance-model-savefile output/oficina3.wrl -radiance-method Galerkin -raytracing-method none -batch -batch-quit-at-end -offscreen
