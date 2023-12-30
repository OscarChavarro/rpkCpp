#!/bin/bash

# 2min43sec on unknown date and computer
# 1min52sec on boobies, jan. 1 2023
# 0m48.578s on buttocks, nov 25 2023
#./build/rpk etc/corridor.mgf -batch -raytracing-method none -iterations 100 -radiance-method Galerkin -radiance-model-savefile output/corridor.wrl -eyepoint 4.78 -10.7 8 -center 4.8 -1 5.62 -batch-quit-at-end -offscreen -raycast -radiance-image-savefile ./output/corridor.ppm

./build/rpk etc/corridor.mgf \
	    -raytracing-method none \
	    -nqcdivs 16 -iterations 100 -radiance-method Galerkin \
	    -radiance-model-savefile output/corridor.wrl \
	    -eyepoint 4.78 -10.7 8 -center 4.8 -1 5.62 \
	    -batch -batch-quit-at-end -offscreen \
	    -raycast -radiance-image-savefile ./output/corridor.ppm

