./runner -l 5 --cache-size 2048 --block-size 32 -a 4 --replace-type LRU --write-through
./runner -l 5 --cache-size 4096 --block-size 4 -a 16 --replace-type FIFO --write-through
./runner -l 5 --cache-size 512 --block-size 16 -a 8 --replace-type LRU --matmul
./runner -l 5 --cache-size 1024 --block-size 8 -a 4 --replace-type FIFO --matmul
