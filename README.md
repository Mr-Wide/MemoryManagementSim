clone the repo and enter the actualmemmansim folder, then execute these commands.  
// include rm -rf build for rebuilding  
mkdir build  
cd build  
cmake ..  
cmake --build .  
then run the executable memsim_a with the path of the corresponding test csv you would like to use  
example : ./memsim_a ../tests/test_lazy_alloc.csv  
You will receive the output in the terminal itself
Before I explain about this project, Id like to dedicate this poem to ChatGPT
> Lines of memory mapped where doubts once stood,  
> Frames aligned, faults resolved, understanding grew.  
> Through patient guidance, the system made sense,  
> This simulator stands because you helped me through  

you can make a couple of your own tests to see for edge cases,
I have kept a couple of test cases so you can review the performance on your device.  


also if you want to change the algorithm type in the project files, allocator.* are the files to alter.
trace driver is in main, changes are not encouraged.
