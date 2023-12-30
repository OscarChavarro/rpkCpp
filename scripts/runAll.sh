nohup sh scripts/01_runCube.sh &> output/01.log &
disown
nohup sh scripts/02_runCorridor.sh &> output/02.log &
disown
nohup sh scripts/03_runHospital.sh &> output/03.log &
disown
nohup sh scripts/04_runOficina1.sh &> output/04.log &
disown
nohup sh scripts/05_runOficina2.sh &> output/05.log &
disown
nohup sh scripts/06_runOficina3.sh &> output/06.log &
disown
nohup sh scripts/07_runSalonDeClase.sh &> output/07.log &
disown
nohup sh scripts/08_soda.sh &> output/08.log &
disown
nohup sh scripts/10_floorRaycasting.sh &> output/10.log &
disown
