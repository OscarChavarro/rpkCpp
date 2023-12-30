#!/bin/bash

mkdir -p output

./build/rpk etc/hospital/hosp.mgf -batch -raytracing-method none -iterations 9 -radiance-method Galerkin -radiance-model-savefile output/hospital.wrl -eyepoint 1.1769 -0.045 1.5556 -center 5.2872 9.0366 0.9494 -offscreen
