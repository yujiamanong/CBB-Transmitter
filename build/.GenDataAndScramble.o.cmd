cmd_GenDataAndScramble.o = gcc -Wp,-MD,./.GenDataAndScramble.o.d.tmp  -m64 -pthread  -march=native -DRTE_MACHINE_CPUFLAG_SSE -DRTE_MACHINE_CPUFLAG_SSE2 -DRTE_MACHINE_CPUFLAG_SSE3 -DRTE_MACHINE_CPUFLAG_SSSE3 -DRTE_MACHINE_CPUFLAG_SSE4_1 -DRTE_MACHINE_CPUFLAG_SSE4_2 -DRTE_MACHINE_CPUFLAG_AES -DRTE_MACHINE_CPUFLAG_PCLMULQDQ -DRTE_MACHINE_CPUFLAG_AVX -DRTE_MACHINE_CPUFLAG_RDRAND -DRTE_MACHINE_CPUFLAG_FSGSBASE -DRTE_MACHINE_CPUFLAG_F16C -DRTE_MACHINE_CPUFLAG_AVX2  -I/home/yujiamanong/Desktop/CBB-Transmitter/build/include -I/home/yujiamanong/Desktop/dpdk-stable-16.11.1/x86_64-native-linuxapp-gcc/include -include /home/yujiamanong/Desktop/dpdk-stable-16.11.1/x86_64-native-linuxapp-gcc/include/rte_config.h -O1 -I/home/yujiamanong/Desktop/CBB-Transmitter/BCCencode -I/home/yujiamanong/Desktop/CBB-Transmitter/intrinsics_interface -I/home/yujiamanong/Desktop/CBB-Transmitter/Process -I/home/yujiamanong/Desktop/CBB-Transmitter/process_data -I/home/yujiamanong/Desktop/CBB-Transmitter/process_data/process_datafunction -I/home/yujiamanong/Desktop/CBB-Transmitter/typeDef -I/home/yujiamanong/Desktop/CBB-Transmitter/VarINIT -D OPTIMIZATION    -o GenDataAndScramble.o -c /home/yujiamanong/Desktop/CBB-Transmitter/process_data/process_datafunction/GenDataAndScramble.c 
