# Para rodar na primeira vez
mkdir build
cd build

cmake ..
make -j8
make run

# Para rodar nas outras vezes
make clear
cmake ..
make -j8
make run  