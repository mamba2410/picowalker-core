echo "Cleaning Project & Building RP2040"
rm -rf build/rp2040
cmake -B build/rp2040 -DCMAKE_TOOLCHAIN_FILE=./toolchain-pico.cmake .
cmake --build build/rp2040
echo "RP2040 Picowalker-core.a Complete!"

echo "Cleaning Project & Building RP2350"
rm -rf build/rp2350
cmake -B build/rp2350 -DCMAKE_TOOLCHAIN_FILE=./toolchain-pico2.cmake .
cmake --build build/rp2350
echo "RP2350 Picowalker-core.a Complete!"