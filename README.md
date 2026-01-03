clone the repo and enter the actualmemmansim folder, then execute these commands.
rm -rf build
mkdir build
cd build
cmake ..
cmake --build .
then run the executable memsim_a with the path of the corresponding test csv you would like to use
example : ./memsim_a ../tests/test_lazy_alloc.csv
You will receive the output in the terminal itself
