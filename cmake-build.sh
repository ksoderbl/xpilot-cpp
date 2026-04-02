mkdir build
cd build
cmake .. -DXPILOT_BUILD_SDL_CLIENT=ON -DXPILOT_USE_SDL_GAMELOOP=OFF
cmake --build . -j
