../utils/pintos -v -k -T 60 --qemu  --filesys-size=2 -p build/tests/vm/mmap-read -a mmap-read -p ../tests/vm/sample.txt -a sample.txt --swap-size=4 -- -q  -f run mmap-read
