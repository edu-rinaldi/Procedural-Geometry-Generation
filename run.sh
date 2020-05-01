./bin/Release/yscenegen tests/01_terrain/terrain.json -o outs/01_terrain/terrain.json --terrain object
./bin/Release/yscenegen tests/02_displacement/displacement.json -o outs/02_displacement/displacement.json --displacement object
./bin/Release/yscenegen tests/03_hair1/hair1.json -o outs/03_hair1/hair1.json --hairbase object --hair hair
./bin/Release/yscenegen tests/03_hair2/hair2.json -o outs/03_hair2/hair2.json --hairbase object --hair hair --hairlen 0.005 --hairstr 0
./bin/Release/yscenegen tests/03_hair3/hair3.json -o outs/03_hair3/hair3.json --hairbase object --hair hair --hairlen 0.005 --hairstr 0.01
./bin/Release/yscenegen tests/03_hair4/hair4.json -o outs/03_hair4/hair4.json --hairbase object --hair hair --hairlen 0.02 --hairstr 0.001 --hairgrav 0.0005 --hairstep 8
./bin/Release/yscenegen tests/04_grass/grass.json -o outs/04_grass/grass.json --grassbase object --grass grass

./bin/Release/yscenetrace outs/01_terrain/terrain.json -o out/01_terrain.png -s 256 -r 720
./bin/Release/yscenetrace outs/02_displacement/displacement.json -o out/02_displacement.png -s 256 -r 720
./bin/Release/yscenetrace outs/03_hair1/hair1.json -o out/03_hair1.png -s 256 -r 720
./bin/Release/yscenetrace outs/03_hair2/hair2.json -o out/03_hair2.png -s 256 -r 720
./bin/Release/yscenetrace outs/03_hair3/hair3.json -o out/03_hair3.png -s 256 -r 720
./bin/Release/yscenetrace outs/03_hair4/hair4.json -o out/03_hair4.png -s 256 -r 720
./bin/Release/yscenetrace outs/04_grass/grass.json -o out/04_grass.png -s 256 -r 720 -b 128
